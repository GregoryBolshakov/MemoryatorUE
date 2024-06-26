#include "MHostileMobController.h"

#include "Components/M2DRepresentationComponent.h"
#include "Components/MAttackPuddleComponent.h"
#include "Characters/MMemoryator.h"
#include "Characters/MMob.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Managers/MWorldGenerator.h"
#include "NavigationSystem.h"
#include "Components/MStateModelComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MStatsModelComponent.h"
#include "Engine/DamageEvents.h"
#include "Framework/MGameMode.h"

AMHostileMobController::AMHostileMobController(const FObjectInitializer& ObjectInitializer) :
	  Super(ObjectInitializer)
	, Victim(nullptr)
{
}

void AMHostileMobController::DoIdleBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	const auto WorldGenerator = AMGameMode::GetWorldGenerator(this);
	if (!WorldGenerator)
	{
		check(false);
		return;
	}

	const auto MyLocation = MyCharacter.GetTransform().GetLocation();
	//TODO: Optimise the GetActorsInRect() to return elements by Class or Tag, etc.
	const auto SightRange = MyCharacter.GetStatsModelComponent()->GetSightRange();
	const auto DynamicActorsInSight = WorldGenerator->GetActorsInRect(MyLocation - FVector(SightRange,SightRange, 0.f), MyLocation + FVector(SightRange,SightRange, 0.f), true);

	if (!DynamicActorsInSight.IsEmpty())
	{
		for (const auto& [Name, DynamicActor] : DynamicActorsInSight)
		{
			//TODO: add a list of enemy/neutral/friends. possibly a map with tags o class names
			if (DynamicActor && DynamicActor != this && DynamicActor->GetClass()->GetSuperClass()->IsChildOf(AMMemoryator::StaticClass()))
			{
				const auto DistanceToActor = FVector::Distance(DynamicActor->GetTransform().GetLocation(), MyCharacter.GetTransform().GetLocation());
				if (DistanceToActor <= MyCharacter.GetStatsModelComponent()->GetSightRange())
				{
					Victim = Cast<APawn>(DynamicActor);
					SetChaseBehavior(World, MyCharacter);
					break;
				}
			}
		}
	}
}

void AMHostileMobController::DoChaseBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	if (!Victim)
	{
		check(false);
		SetIdleBehavior(&World, &MyCharacter);
		return;
	}

	const auto DistanceToVictim = FVector::Distance(Victim->GetTransform().GetLocation(), MyCharacter.GetTransform().GetLocation());
	if (DistanceToVictim > MyCharacter.GetStatsModelComponent()->GetForgetEnemyRange())
	{
		SetIdleBehavior(&World, &MyCharacter);
		return;
	}

	float VictimRadius = 0.f;
	if (const auto VictimCapsule = Cast<UCapsuleComponent>(Victim->GetRootComponent()))
	{
		VictimRadius = VictimCapsule->GetScaledCapsuleRadius();
	}

	const auto MyMob = Cast<AMMob>(&MyCharacter);
	const float PileInLength = MyMob ? MyMob->GetPileInLength() : 0.f;

	// For reliability, update the move goal
	MoveToLocation(Victim->GetActorLocation(), MyCharacter.GetStatsModelComponent()->GetFightRangePlusRadius(MyCharacter.GetRadius()) + VictimRadius - PileInLength, false);

	//TODO: Add a logic to do during chase (shouts, effects, etc.)
}

void AMHostileMobController::DoFightBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	if (!Victim)
	{
		check(false);
		SetIdleBehavior(&World, &MyCharacter);
		return;
	}

	// Face the victim during the strike
	//TODO: This cause continuing facing even if victim bypassed us. Maybe it's unnecessary
	const auto GazeVector = Victim->GetTransform().GetLocation() - MyCharacter.GetTransform().GetLocation();
	MyCharacter.SetForcedGazeVector(GazeVector);
	//TODO: Add a logic to do during fight (shouts, effects, etc.)
}

void AMHostileMobController::DoRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	if (!Victim)
	{
		check(false);
		SetIdleBehavior(&World, &MyCharacter);
		return;
	}

	// if we have already run back a sufficient length, then there is no need to run away anymore
	const auto DistanceToVictim = FVector::Distance(Victim->GetTransform().GetLocation(), MyCharacter.GetTransform().GetLocation());
	if (DistanceToVictim >= MyCharacter.GetStatsModelComponent()->GetRetreatRange())
	{
		SetChaseBehavior(World, MyCharacter);
	}
}

void AMHostileMobController::SetIdleBehavior(const UWorld* World, AMCharacter* MyCharacter)
{
	MyCharacter->GetStateModelComponent()->SetIsFighting(false);
	MyCharacter->GetStateModelComponent()->SetIsMoving(false);

	StopMovement();

	CurrentBehavior = EMobBehaviors::Idle;
	Victim = nullptr;

	OnBehaviorChanged(*MyCharacter);
}

void AMHostileMobController::SetChaseBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	MyCharacter.GetStateModelComponent()->SetIsFighting(false);
	MyCharacter.GetStateModelComponent()->SetIsMoving(true);

	CurrentBehavior = EMobBehaviors::Chase;

	if (!Victim)
	{
		check(false);
		SetIdleBehavior(&World, &MyCharacter);
		return;
	}

	// Reset forced gaze. Facing move direction (it is the default case)
	MyCharacter.SetForcedGazeVector(FVector::ZeroVector);

	OnMoveCompletedDelegate.Unbind();
	OnMoveCompletedDelegate.BindLambda([this, &World, &MyCharacter]
	{
		SetFightBehavior(World, MyCharacter);
	});

	float VictimRadius = 0.f;
	if (const auto VictimCapsule = Cast<UCapsuleComponent>(Victim->GetRootComponent()))
	{
		VictimRadius = VictimCapsule->GetScaledCapsuleRadius();
	}

	const auto MyMob = Cast<AMMob>(&MyCharacter);
	const float PileInLength = MyMob ? MyMob->GetPileInLength() : 0.f;

	MoveToLocation(Victim->GetActorLocation(), MyCharacter.GetStatsModelComponent()->GetFightRangePlusRadius(MyCharacter.GetRadius()) + VictimRadius - PileInLength, false);

	OnBehaviorChanged(MyCharacter);
}

void AMHostileMobController::SetFightBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	if (!Victim)
	{
		check(false);
		SetIdleBehavior(&World, &MyCharacter);
		return;
	}

	MyCharacter.GetStateModelComponent()->SetIsFighting(true);
	MyCharacter.GetStateModelComponent()->SetIsMoving(false);

	CurrentBehavior = EMobBehaviors::Fight;

	OnBehaviorChanged(MyCharacter);

	// Face the victim during the strike
	const auto GazeVector = Victim->GetTransform().GetLocation() - MyCharacter.GetTransform().GetLocation();
	MyCharacter.SetForcedGazeVector(GazeVector);
}

void AMHostileMobController::SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	MyCharacter.GetStateModelComponent()->SetIsFighting(false);
	MyCharacter.GetStateModelComponent()->SetIsMoving(true);

	CurrentBehavior = EMobBehaviors::Retreat;

	if (!Victim)
	{
		// We need to know who are we running from
		check(false);
		SetIdleBehavior(&World, &MyCharacter);
		return;
	}

	// Calculate end point of retreat
	const auto MyLocation = MyCharacter.GetTransform().GetLocation();
	auto RetreatDirection = MyLocation - Victim->GetTransform().GetLocation();
	RetreatDirection.Normalize();
	RetreatDirection *= MyCharacter.GetStatsModelComponent()->GetRetreatRange();
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
		SetIdleBehavior(&World, &MyCharacter);
		return;
	}

	OnBehaviorChanged(MyCharacter);
}

void AMHostileMobController::OnBehaviorChanged(AMCharacter& MyCharacter)
{
	// TODO: Consider removing this function
}

void AMHostileMobController::OnFightAnimationEnd()
{
	const auto MyCharacter = Cast<AMCharacter>(GetPawn());
	if (!MyCharacter)
	{
		check(false);
		return;
	}

	if (MyCharacter->GetStatsModelComponent()->GetCanRetreat())
	{
		SetRetreatBehavior(*GetWorld(), *MyCharacter);
	}
	else
	{
		SetIdleBehavior(GetWorld(), MyCharacter);
	}
}

void AMHostileMobController::OnHit()
{
	check(HasAuthority());
	const auto MyCharacter = Cast<AMCharacter>(GetPawn());
	if (!MyCharacter)
		return;

	const auto AttackPuddleComponent = MyCharacter->GetAttackPuddleComponent();
	if (!AttackPuddleComponent)
		return;

	// Make sure we rotate towards victim at the moment of hit
	DoFightBehavior(*GetWorld(), *MyCharacter);
	MyCharacter->Tick(0.f);
	AttackPuddleComponent->UpdateRotation();

	TArray<AActor*> OutActors;
	UKismetSystemLibrary::BoxOverlapActors(GetWorld(), AttackPuddleComponent->GetComponentLocation(), AttackPuddleComponent->Bounds.BoxExtent, {UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn)}, AMCharacter::StaticClass(), {MyCharacter}, OutActors);
	for (const auto Actor : OutActors)
	{
		if (!Actor)
			continue;

		if (const auto CapsuleComponent = Cast<UCapsuleComponent>(Actor->GetRootComponent()))
		{
			if (AttackPuddleComponent->IsCircleWithin(Actor->GetActorLocation(), CapsuleComponent->GetScaledCapsuleRadius()))
			{
				Actor->TakeDamage(MyCharacter->GetStatsModelComponent()->GetStrength(), FDamageEvent(), this, MyCharacter);
			}
		}
	}
}

