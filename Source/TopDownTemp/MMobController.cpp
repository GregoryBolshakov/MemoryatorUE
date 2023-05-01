#include "MMobController.h"

#include "M2DRepresentationComponent.h"
#include "MMemoryator.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MWorldManager.h"
#include "MWorldGenerator.h"
#include "NavigationSystem.h"

AMMobController::AMMobController(const FObjectInitializer& ObjectInitializer) :
	  Super(ObjectInitializer)
	, Victim(nullptr)
{
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
		for (const auto& [Name, DynamicActor] : DynamicActorsInSight)
		{
			//TODO: add a list of enemy/neutral/friends. possibly a map with tags o class names
			if (DynamicActor && DynamicActor != this && DynamicActor->GetClass()->GetSuperClass()->IsChildOf(AMMemoryator::StaticClass()))
			{
				const auto DistanceToActor = FVector::Distance(DynamicActor->GetTransform().GetLocation(), MyCharacter.GetTransform().GetLocation());
				if (DistanceToActor <= MyCharacter.GetSightRange())
				{
					Victim = Cast<APawn>(DynamicActor);
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
	if (DistanceToVictim > MyCharacter.GetForgetEnemyRange())
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

	// Reset forced gaze. Facing move direction (it is the default case)
	MyCharacter.SetForcedGazeVector(FVector::ZeroVector);

	OnMoveCompletedDelegate.Unbind();
	OnMoveCompletedDelegate.BindLambda([this, &World, &MyCharacter]
	{
		SetFightBehavior(World, MyCharacter);
	});
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
		OnMoveCompletedDelegate.Unbind();
		OnMoveCompletedDelegate.BindLambda([this, &World, &MyCharacter]
		{
			SetChaseBehavior(World, MyCharacter);
		});
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
	MyCharacter.UpdateAnimation();
}

void AMMobController::OnFightAnimationEnd()
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

