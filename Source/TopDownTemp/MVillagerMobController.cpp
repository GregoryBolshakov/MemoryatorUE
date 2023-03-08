#include "MVillagerMobController.h"

#include "MMemoryator.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MWorldManager.h"
#include "MWorldGenerator.h"
#include "NavigationSystem.h"

AMVillagerMobController::AMVillagerMobController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
}

void AMVillagerMobController::Tick(float DeltaSeconds)
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

void AMVillagerMobController::DoIdleBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	const auto pWorldGenerator = World.GetSubsystem<UMWorldManager>()->GetWorldGenerator();
	if (!pWorldGenerator)
	{
		check(false);
		return;
	}

	/*const auto MyLocation = MyCharacter.GetTransform().GetLocation();
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
	}*/
}

void AMVillagerMobController::DoWalkBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
}

void AMVillagerMobController::DoRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	/*if (!Victim)
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
	}*/
}

void AMVillagerMobController::SetIdleBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	/*MyCharacter.SetIsFighting(false);
	MyCharacter.SetIsMoving(false);

	StopMovement();

	CurrentBehavior = EMobBehaviors::Idle;
	Victim = nullptr;

	OnBehaviorChanged(MyCharacter);*/
}

void AMVillagerMobController::SetWalkBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
}

void AMVillagerMobController::SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	/*MyCharacter.SetIsFighting(false);
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

	OnBehaviorChanged(MyCharacter);*/
}

void AMVillagerMobController::OnBehaviorChanged(AMCharacter& MyCharacter)
{
	MyCharacter.SetForcedGazeVector(FVector::ZeroVector);

	MyCharacter.UpdateAnimation();
}

void AMVillagerMobController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
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

