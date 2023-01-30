#include "MMobController.h"

#include "MMemoryator.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MWorldManager.h"
#include "MWorldGenerator.h"

AMMobController::AMMobController(const FObjectInitializer& ObjectInitializer) :
	  Super(ObjectInitializer)
	, CurrentBehavior()
	, Victim(nullptr)
	, DefaultTimeBetweenDecisions(0.5f)
	, CurrentTimeBetweenDecisions(0.f)
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AMMobController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// For better performance the behavior decisions are made once in a while
	CurrentTimeBetweenDecisions -= DeltaSeconds;
	if (CurrentTimeBetweenDecisions > 0.f)
	{
		return;
	}

	const auto pWorld = GetWorld();
	if (!pWorld)
	{
		check(false);
		return;
	}
	
	const auto MyCharacter = Cast<AMCharacter>(GetPawn());
	if (!MyCharacter)
	{
		check(false);
		return;
	}

	// Reset ForcedGazeVector, it might be set in one of behavior functions 
	MyCharacter->SetForcedGazeVector(FVector::ZeroVector);

	if (CurrentBehavior == EMobBehaviors::Idle)
	{
		// Try to find a victim in sight distance. If there is one, start chasing it
		DoIdleBehavior(*pWorld, *MyCharacter);
	}
	if (CurrentBehavior == EMobBehaviors::Chase)
	{
		// Chase the victim until get within strike distance. Then try to strike
		DoChaseBehavior(*pWorld, *MyCharacter);
	}
	if (CurrentBehavior == EMobBehaviors::Fight)
	{
		// Fight the victim and return to the Idle behavior
		DoFightBehavior(*pWorld, *MyCharacter);
	}
}

void AMMobController::DoIdleBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	const auto pWorldGenerator = World.GetSubsystem<UMWorldManager>()->GetWorldGenerator();
	if (!pWorldGenerator)
	{
		check(false);
		return;
	}

	const auto MyLocation = MyCharacter.GetTransform().GetLocation();
	//TODO: Optimise the GetActorsInRect() to return elements by Class or Tag, etc.
	const auto SightRange = MyCharacter.GetSightRange();
	const auto DynamicActorsInSight = pWorldGenerator->GetActorsInRect(MyLocation - FVector(SightRange,SightRange, 0.f), MyLocation + FVector(SightRange,SightRange, 0.f), true);

	if (!DynamicActorsInSight.IsEmpty())
	{
		for (const auto& [Name, Metadata] : DynamicActorsInSight)
		{
			//TODO: add a list of enemy/neutral/friends. possibly a map with tags o class names
			if (Metadata.Actor != this && Metadata.Actor->GetClass()->GetSuperClass()->IsChildOf(AMMemoryator::StaticClass()))
			{
				const auto DistanceToActor = FVector::Distance(Metadata.Actor->GetTransform().GetLocation(), MyCharacter.GetTransform().GetLocation());
				if (DistanceToActor <= MyCharacter.GetSightRange())
				{
					Victim = Cast<APawn>(Metadata.Actor);
					SetChaseBehavior(World, MyCharacter);
					break;
				}
			}
		}
	}
}

void AMMobController::DoChaseBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	if (!Victim)
	{
		check(false);
		SetIdleBehavior(World, MyCharacter);
		return;
	}

	const auto DistanceToVictim = FVector::Distance(Victim->GetTransform().GetLocation(), MyCharacter.GetTransform().GetLocation());
	if (DistanceToVictim > MyCharacter.GetSightRange() * 1.5f) //TODO: come up with "1.5f" parameter name
	{
		SetIdleBehavior(World, MyCharacter);
		Victim = nullptr;
		return;
	}

	// For reliability, update the move goal
	MoveToActor(Victim, MyCharacter.GetFightRange());

	//TODO: Add a logic to do during chase (shouts, effects, etc.)
}

void AMMobController::DoFightBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	if (!Victim)
	{
		check(false);
		SetIdleBehavior(World, MyCharacter);
		return;
	}

	// Face the victim during the strike
	//TODO: This cause continuing facing even if victim bypassed us. Maybe it's unnecessary
	const auto GazeVector = Victim->GetTransform().GetLocation() - MyCharacter.GetTransform().GetLocation();
	MyCharacter.SetForcedGazeVector(GazeVector);
	//TODO: Add a logic to do during fight (shouts, effects, etc.)
}

void AMMobController::SetIdleBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	MyCharacter.SetIsFighting(false);

	StopMovement();

	CurrentBehavior = EMobBehaviors::Idle;

	OnBehaviorChanged(MyCharacter);
}

void AMMobController::SetChaseBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	MyCharacter.SetIsFighting(false);

	CurrentBehavior = EMobBehaviors::Chase;

	if (!Victim)
	{
		check(false);
		SetIdleBehavior(World, MyCharacter);
		return;
	}

	MoveToActor(Victim, MyCharacter.GetFightRange());

	OnBehaviorChanged(MyCharacter);
}

void AMMobController::SetFightBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	if (!Victim)
	{
		check(false);
		SetIdleBehavior(World, MyCharacter);
		return;
	}

	MyCharacter.SetIsFighting(true);
	// Face the victim during the strike
	const auto GazeVector = Victim->GetTransform().GetLocation() - MyCharacter.GetTransform().GetLocation();
	MyCharacter.SetForcedGazeVector(GazeVector);

	CurrentBehavior = EMobBehaviors::Fight;

	OnBehaviorChanged(MyCharacter);
}

void AMMobController::SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
}

void AMMobController::OnBehaviorChanged(AMCharacter& MyCharacter)
{
	CurrentTimeBetweenDecisions = DefaultTimeBetweenDecisions;

	MyCharacter.UpdateAnimation();
}

void AMMobController::OnFightEnd()
{
	const auto MyCharacter = Cast<AMCharacter>(GetPawn());
	if (!MyCharacter)
	{
		check(false);
		return;
	}

	SetIdleBehavior(*GetWorld(), *MyCharacter);
}

void AMMobController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (!Result.IsSuccess())
	{
		return;
	}

	const auto pWold = GetWorld();
	if (!pWold)
	{
		check(false);
		return;
	}

	const auto MyCharacter = Cast<AMCharacter>(GetPawn());
	if (!MyCharacter)
	{
		check(false);
		return;
	}

	switch (CurrentBehavior)
	{
	case EMobBehaviors::Chase:
	default:
		SetFightBehavior(*pWold, *MyCharacter);
		return;
	case EMobBehaviors::Follow:
		SetIdleBehavior(*pWold, *MyCharacter);
		return;
	}
}

