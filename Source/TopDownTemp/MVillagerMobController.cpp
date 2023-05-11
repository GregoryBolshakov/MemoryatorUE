#include "MVillagerMobController.h"

#include "MActor.h"
#include "MInventoryComponent.h"
#include "MIsActiveCheckerComponent.h"
#include "MMemoryator.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MWorldManager.h"
#include "MWorldGenerator.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"

AMVillagerMobController::AMVillagerMobController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, HomeBuilding(nullptr)
	, VillageRadius(0)
	, bEmbarked(false)
{
}

void AMVillagerMobController::PreTick(float DeltaSeconds, const UWorld& World, AMCharacter& MyCharacter)
{
	return;
	if (const auto pWorldGenerator = World.GetSubsystem<UMWorldManager>()->GetWorldGenerator())
	{
		const auto MyLocation = MyCharacter.GetTransform().GetLocation();
		const auto ForgetEnemyRange = MyCharacter.GetForgetEnemyRange();
		const auto DynamicActorsNearby = pWorldGenerator->GetActorsInRect(MyLocation - FVector(ForgetEnemyRange,ForgetEnemyRange, 0.f), MyLocation + FVector(ForgetEnemyRange,ForgetEnemyRange, 0.f), true);
		EnemiesNearby.Empty();

		if (!DynamicActorsNearby.IsEmpty())
		{
			for (const auto& [Name, DynamicActor] : DynamicActorsNearby)
			{
				// The actors are taken in a square area, in the corners the distance is greater than the radius
				const auto DistanceToActor = FVector::Distance(DynamicActor->GetTransform().GetLocation(), MyCharacter.GetTransform().GetLocation());
				if (DistanceToActor <= MyCharacter.GetForgetEnemyRange())
				{
					// Split dynamic actors by role

					// Check if the actor is an enemy
					if (const auto Relationship = RelationshipMap.Find(DynamicActor->GetClass());
						Relationship && *Relationship == ERelationType::Enemy)
					{
						EnemiesNearby.Add(Name, DynamicActor);
						// Run if we see an enemy. There is no need to run away if we're already hiding
						if (DistanceToActor <= MyCharacter.GetSightRange() && CurrentBehavior != EMobBehaviors::Hide)
						{
							SetRetreatBehavior(World, MyCharacter);
							break;
						}
					}
					
					//TODO: Check if the actor is a friend
				}
			}
		}
	}
}

void AMVillagerMobController::Initialize(AActor& _HomeBuilding, const FVector& _VillageCenter, float _VillageRadius)
{
	HomeBuilding = &_HomeBuilding;
	VillageCenter = _VillageCenter;
	VillageRadius = _VillageRadius;

	if (const auto MyCharacter = Cast<AMCharacter>(GetCharacter()))
	{
		if (const auto Inventory = MyCharacter->GetInventoryComponent())
		{
			Inventory->Initialize(3, {{1, 1}, {2, 2}, {3, 3}});
			Inventory->MakeAllItemsSecret();
			Inventory->LockAllItems();
		}
	}
}

void AMVillagerMobController::DoIdleBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	MyCharacter.SetIsFighting(false);
	MyCharacter.SetIsMoving(false);

	if (auto& TimerManager = GetWorld()->GetTimerManager();
		!TimerManager.IsTimerActive(RestTimerHandle))
	{
		TimerManager.SetTimer(RestTimerHandle, [this, &World, &MyCharacter]
		{
			if (CurrentBehavior != EMobBehaviors::Idle) // If were interrupted by something more important (like retreat from enemy..)
				return;

			constexpr int TriesToFindLocation = 3;
			bool bSuccess = false;
			for (int i = 0; i < TriesToFindLocation; ++i)
			{
				// Calculate a random point within the village to go
				const float RandomAngle = FMath::RandRange(0.f, 1.f) * 2.0f * PI;
				const float RandomRadius = FMath::RandRange(0.f, 1.f) * VillageRadius;
				const FVector RandomPoint{
					VillageCenter.X + RandomRadius * cos(RandomAngle),
					VillageCenter.Y + RandomRadius * sin(RandomAngle),
					0.f
					};

				// Project the destination point to navigation mesh to find the closest reachable node
				FVector NavigatedLocation;
				if (UNavigationSystemV1::K2_ProjectPointToNavigation(const_cast<UWorld*>(&World), RandomPoint, NavigatedLocation, nullptr, nullptr))
				{
					SetWalkBehavior(World, MyCharacter, NavigatedLocation);
					bSuccess = true;
					break;
				}
			}
			if (!bSuccess)
			{
				// If all the points were obstructed, it's okay, villager will wait for the timer again and then try
				SetIdleBehavior(&World, &MyCharacter);
			}
		}, FMath::RandRange(MinRestDuration, MaxRestDuration), false);
	}
}

void AMVillagerMobController::DoWalkBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	//TODO: Add a logic to do during chase (shouts, effects, etc.)
}

void AMVillagerMobController::DoRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	//TODO: Add a logic to do during chase (shouts, effects, etc.)etChaseBehavior(World, MyCharacter);
}

void AMVillagerMobController::DoHideBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	check(bEmbarked);
	if (EnemiesNearby.IsEmpty())
	{
		Disembark(MyCharacter);
		SetIdleBehavior(&World, &MyCharacter);
	}
}

void AMVillagerMobController::SetIdleBehavior(const UWorld* World, AMCharacter* MyCharacter)
{
	MyCharacter->SetIsFighting(false);
	MyCharacter->SetIsMoving(false);

	GetWorld()->GetTimerManager().ClearTimer(RestTimerHandle);

	StopMovement();

	CurrentBehavior = EMobBehaviors::Idle;

	OnBehaviorChanged(*MyCharacter);
}

void AMVillagerMobController::SetWalkBehavior(const UWorld& World, AMCharacter& MyCharacter, const FVector& DestinationPoint)
{
	MyCharacter.SetIsFighting(false);
	MyCharacter.SetIsMoving(true);

	CurrentBehavior = EMobBehaviors::Walk;

	MyCharacter.GetCharacterMovement()->MaxWalkSpeed = MyCharacter.GetWalkSpeed();

	OnMoveCompletedDelegate.Unbind();
	OnMoveCompletedDelegate.BindLambda([this, &World, &MyCharacter]
	{
		if (CurrentBehavior != EMobBehaviors::Walk) // If were interrupted by something more important (like retreat from enemy..)
			return;

		SetIdleBehavior(&World, &MyCharacter);
	});
	MoveToLocation(DestinationPoint);

	OnBehaviorChanged(MyCharacter);
}

void AMVillagerMobController::SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	MyCharacter.SetIsFighting(false);
	MyCharacter.SetIsMoving(true);

	CurrentBehavior = EMobBehaviors::Retreat;

	MyCharacter.GetCharacterMovement()->MaxWalkSpeed = MyCharacter.GetSprintSpeed();

	if (const auto EntryPointComponent = Cast<USceneComponent>(HomeBuilding->GetDefaultSubobjectByName(TEXT("EntryPoint"))))
	{
		OnMoveCompletedDelegate.Unbind();
		StopMovement();
		MyCharacter.GetMovementComponent()->StopMovementImmediately(); // In case of slip or other velocity modificators 

		OnMoveCompletedDelegate.BindLambda([this, &World, &MyCharacter]
		{
			if (CurrentBehavior != EMobBehaviors::Retreat) // If the movement was started not to retreat
				return;

			if (HomeBuilding)
			{
				SetHideBehavior(World, MyCharacter);
			}
			else
			{
				SetIdleBehavior(&World, &MyCharacter);
				//TODO: implement new home assignment
			}
		});

		FVector MyCharacterOrigin, MyCharacterBoxExtent;
		MyCharacter.GetActorBounds(true, MyCharacterOrigin, MyCharacterBoxExtent, true);
		MoveToLocation(EntryPointComponent->GetComponentLocation(), /*MyCharacterBoxExtent.Size2D()*/ 20.f); //TODO: remove this workaround
	}

	OnBehaviorChanged(MyCharacter);
}

void AMVillagerMobController::SetHideBehavior(const UWorld& World, AMCharacter& MyCharacter)
{
	Embark(MyCharacter);
	//TODO: change the building state somehow (track the amount of people inside)

	CurrentBehavior = EMobBehaviors::Hide;

	OnBehaviorChanged(MyCharacter);
}

void AMVillagerMobController::OnBehaviorChanged(AMCharacter& MyCharacter)
{
	MyCharacter.SetForcedGazeVector(FVector::ZeroVector);

	MyCharacter.UpdateAnimation();
}

void AMVillagerMobController::Embark(const AMCharacter& MyCharacter)
{
	if (const auto ActiveCheckerComponent = MyCharacter.GetIsActiveCheckerComponent())
	{
		ActiveCheckerComponent->DisableOwner(true);
	}
	bEmbarked = true;
}

void AMVillagerMobController::Disembark(const AMCharacter& MyCharacter)
{
	if (const auto ActiveCheckerComponent = MyCharacter.GetIsActiveCheckerComponent())
	{
		ActiveCheckerComponent->EnableOwner(true);
	}
	bEmbarked = false;
}

