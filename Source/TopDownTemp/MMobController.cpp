#include "MMobController.h"

#include "MMemoryator.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MWorldManager.h"
#include "MWorldGenerator.h"
#include "NavigationSystem.h"

AMMobController::AMMobController(const FObjectInitializer& ObjectInitializer) :
	  Super(ObjectInitializer)
	, CurrentBehavior()
	, Victim(nullptr)
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
}

void AMMobController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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
	if (CurrentBehavior == EMobBehaviors::Retreat)
	{
		// Run away from the victim until move away by a set range
		DoRetreatBehavior(*pWorld, *MyCharacter);
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
			if (Metadata.Actor && Metadata.Actor != this && Metadata.Actor->GetClass()->GetSuperClass()->IsChildOf(AMMemoryator::StaticClass()))
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

void AMMobController::DoRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	if (!Victim)
	{
		check(false);
		SetIdleBehavior(World, MyCharacter);
		return;
	}

	// if we have already run back a sufficient length, then there is no need to run away anymore
	const auto DistanceToVictim = FVector::Distance(Victim->GetTransform().GetLocation(), MyCharacter.GetTransform().GetLocation());
	if (DistanceToVictim >= MyCharacter.GetRetreatRange())
	{
		SetChaseBehavior(World, MyCharacter);
	}
}

void AMMobController::SetIdleBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	MyCharacter.SetIsFighting(false);
	MyCharacter.SetIsMoving(false);

	StopMovement();

	CurrentBehavior = EMobBehaviors::Idle;
	Victim = nullptr;

	OnBehaviorChanged(MyCharacter);
}

void AMMobController::SetChaseBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	MyCharacter.SetIsFighting(false);
	MyCharacter.SetIsMoving(true);

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
	MyCharacter.SetIsMoving(false);

	CurrentBehavior = EMobBehaviors::Fight;

	OnBehaviorChanged(MyCharacter);

	// Face the victim during the strike
	const auto GazeVector = Victim->GetTransform().GetLocation() - MyCharacter.GetTransform().GetLocation();
	MyCharacter.SetForcedGazeVector(GazeVector);
}

void AMMobController::SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	MyCharacter.SetIsFighting(false);
	MyCharacter.SetIsMoving(true);

	CurrentBehavior = EMobBehaviors::Retreat;

	if (!Victim)
	{
		// We need to know who are we running from
		check(false);
		SetIdleBehavior(World, MyCharacter);
		return;
	}

	// Calculate end point of retreat
	const auto MyLocation = MyCharacter.GetTransform().GetLocation();
	auto RetreatDirection = MyLocation - Victim->GetTransform().GetLocation();
	RetreatDirection.Normalize();
	RetreatDirection *= MyCharacter.GetRetreatRange();
	const auto RetreatLocation = MyLocation + RetreatDirection;

	// Project the point of retreat to navigation mesh to find the closest reachable node
	FVector NavigatedRetreatLocation;
	if (UNavigationSystemV1::K2_ProjectPointToNavigation(const_cast<UWorld*>(&World), RetreatLocation, NavigatedRetreatLocation, nullptr, nullptr))
	{
		MoveToLocation(NavigatedRetreatLocation);
	}
	else
	{
		SetIdleBehavior(World, MyCharacter);
		return;
	}

	OnBehaviorChanged(MyCharacter);
}

void AMMobController::OnBehaviorChanged(AMCharacter& MyCharacter)
{
	MyCharacter.SetForcedGazeVector(FVector::ZeroVector);

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

	if (MyCharacter->GetCanRetreat())
	{
		SetRetreatBehavior(*GetWorld(), *MyCharacter);
	}
	else
	{
		SetIdleBehavior(*GetWorld(), *MyCharacter);
	}
}

void AMMobController::OnHit()
{
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

	if (!Victim)
	{
		SetIdleBehavior(*pWorld, *MyCharacter);
		return;
	}

	//FPointDamageEvent DamageEvent(DamageAmount, Hit, -ActorForward, UDamageType::StaticClass());
	//Victim->TakeDamage(DamageAmount, DamageEvent, MyPC, MyPC->GetPawn());
}

void AMMobController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	AAIController::OnMoveCompleted(RequestID, Result);

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
	case EMobBehaviors::Retreat:
		SetChaseBehavior(*pWold, *MyCharacter);
		return;
	case EMobBehaviors::Follow:
		SetIdleBehavior(*pWold, *MyCharacter);
		return;
	}
}

