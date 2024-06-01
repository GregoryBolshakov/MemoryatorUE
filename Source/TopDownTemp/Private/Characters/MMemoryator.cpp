#include "MMemoryator.h"

#include "HeadMountedDisplayFunctionLibrary.h"
#include "TopDownTemp.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/MAttackPuddleComponent.h"
#include "Controllers/MInterfaceMobController.h"
#include "Components/MInventoryComponent.h"
#include "Controllers/MPlayerController.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components//MStateModelComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MAbilitySystemComponent.h"
#include "Components/MStatsModelComponent.h"
#include "Framework/MGameMode.h"

AMMemoryator::AMMemoryator(const FObjectInitializer& ObjectInitializer) :
	Super(
		ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("CharacterMesh0"))
		 )
{
	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level
	CameraBoom->SetRelativeRotation(FRotator(0.f, -35.f, 0.f));

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	TopDownCameraComponent->FieldOfView = 60.f;

	// Create a decal in the world to show the cursor's location
	CursorToWorld = CreateDefaultSubobject<USceneComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(RootComponent);
}

void AMMemoryator::AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce)
{
	Super::AddMovementInput(WorldDirection, ScaleValue, bForce);

	if (!FMath::IsNearlyZero(ScaleValue)) // Got movement from Input
	{
		const auto PlayerController = Cast<AMPlayerController>(Controller);
		if (IsValid(PlayerController) && PlayerController->IsMovingByAI())
		{
			PlayerController->StopAIMovement();
		}
	}
}

void AMMemoryator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		// Get actors nearby every N seconds. We don't need to do this every frame
		if (auto& TimerManager = GetWorld()->GetTimerManager();
			!TimerManager.IsTimerActive(ActorsNearbyUpdateTimerHandle))
		{
			TimerManager.SetTimer(ActorsNearbyUpdateTimerHandle, [this]
			{
				SetDynamicActorsNearby();
			}, 1.f, false);
		}

		UpdateClosestEnemy();

		HandleMovementState();

		HandleCursor();
	}
}

void AMMemoryator::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// TODO: Move input binding from AMPlayerController::SetupInputComponent here

	BindASCInput();
}

void AMMemoryator::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	//TODO: Call BindASCInput() when move AbilitySystemComponent to PlayerState
}

void AMMemoryator::BindASCInput()
{
	if (!bASCInputBound && IsValid(AbilitySystemComponent) && IsValid(InputComponent))
	{
		FTopLevelAssetPath AbilityEnumAssetPath = FTopLevelAssetPath(FName("/Script/TopDownTemp"), FName("EMAbilityInputID"));
		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, FGameplayAbilityInputBinds(FString("ConfirmTarget"),
			FString("CancelTarget"), AbilityEnumAssetPath, static_cast<int32>(EMAbilityInputID::Confirm), static_cast<int32>(EMAbilityInputID::Cancel)));

		bASCInputBound = true;
	}
}

void AMMemoryator::SetDynamicActorsNearby()
{
	if (const auto WorldGenerator = AMGameMode::GetWorldGenerator(this))
	{
		const auto CharacterLocation = GetTransform().GetLocation();
		const auto ForgetEnemyRange =GetStatsModelComponent()->GetForgetEnemyRange();
		const auto DynamicActorsNearby = WorldGenerator->GetActorsInRect(
			CharacterLocation - FVector(ForgetEnemyRange, ForgetEnemyRange, 0.f),
			CharacterLocation + FVector(ForgetEnemyRange, ForgetEnemyRange, 0.f), true);
		EnemiesNearby.Empty();

		if (!DynamicActorsNearby.IsEmpty())
		{
			for (const auto& [Name, DynamicActor] : DynamicActorsNearby)
			{
				if (DynamicActor == this)
				{
					continue; // Skip myself
				}
				// The actors are taken in a square area, in the corners the distance is greater than the radius
				const auto DistanceToActor = FVector::Distance(DynamicActor->GetTransform().GetLocation(),
				                                               GetTransform().GetLocation());
				if (DistanceToActor <= GetStatsModelComponent()->GetForgetEnemyRange())
				{
					// Split dynamic actors by role

					// Check if the actor is an enemy
					if (const auto Relationship = RelationshipMap.Find(DynamicActor->GetClass());
						Relationship && *Relationship == ERelationType::Enemy)
					{
						EnemiesNearby.Add(Name, DynamicActor);
					}

					//TODO: Check if the actor is a friend
				}
			}
		}
	}
}

void AMMemoryator::UpdateClosestEnemy()
{
	const auto PuddleComponent = AttackPuddleComponent;
	if (!PuddleComponent)
	{
		return;
	}

	if (StateModelComponent->GetIsDashing()) // check for any action that shouldn't rotate character towards enemy
	{
		PuddleComponent->SetHiddenInGame(true);
		return;
	}

	const auto CharacterLocation = GetTransform().GetLocation();
	bool bEnemyWasValid = IsValid(ClosestEnemy);
	ClosestEnemy = nullptr;

	float ClosestEnemyRadius = 0.f;
	for (const auto [Name, EnemyActor] : EnemiesNearby)
	{
		if (!IsValid(EnemyActor))
		{
			continue;
		}
		const auto Capsule = EnemyActor->FindComponentByClass<UCapsuleComponent>();
		if (!Capsule)
		{
			check(false);
			continue;
		}

		const auto EnemyRadius = Capsule->GetScaledCapsuleRadius();
		const auto EnemyLocation = EnemyActor->GetTransform().GetLocation();
		const auto DistanceToActor = FVector::Distance(CharacterLocation, EnemyLocation);
		// Fix our character's gaze on the enemy if it is the closest one and 
		if (DistanceToActor + EnemyRadius <= StatsModelComponent->GetFightRangePlusRadius(GetRadius()) * 3.5f &&
			// Actor is within sight range TODO: put the " * 2.5f" to the properties
			(!ClosestEnemy || DistanceToActor < FVector::Distance(CharacterLocation, ClosestEnemy->GetActorLocation())))
		// It is either the only one in sight or the closest
		{
			ClosestEnemy = EnemyActor;
			ClosestEnemyRadius = EnemyRadius;
		}
	}

	if (ClosestEnemy)
	{
		const auto VectorToEnemy = ClosestEnemy->GetActorLocation() - CharacterLocation;
		SetForcedGazeVector(VectorToEnemy);
		PuddleComponent->SetHiddenInGame(false);

		if (VectorToEnemy.Size2D() <= StatsModelComponent->GetFightRangePlusRadius(GetRadius()) + ClosestEnemyRadius && !StateModelComponent->GetIsFighting())
		{
			StateModelComponent->SetIsFighting(true);
		}
	}
	else
	{
		SetForcedGazeVector(FVector::ZeroVector);
		PuddleComponent->SetHiddenInGame(true);
		if (bEnemyWasValid && // Enemy was valid but has just become invalid
			StateModelComponent->GetIsMoving()) 
		{
			if (auto* MPlayerController = Cast<AMPlayerController>(GetController()))
			{
				MPlayerController->StartSprintTimer();
			}
		}
	}
}

void AMMemoryator::HandleCursor() const
{
	if (CursorToWorld != nullptr)
	{
		if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			if (UWorld* World = GetWorld())
			{
				FHitResult HitResult;
				FCollisionQueryParams Params(NAME_None, FCollisionQueryParams::GetUnknownStatId());
				FVector StartLocation = TopDownCameraComponent->GetComponentLocation();
				FVector EndLocation = TopDownCameraComponent->GetComponentRotation().Vector() * 2000.0f;
				Params.AddIgnoredActor(this);
				World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);
				FQuat SurfaceRotation = HitResult.ImpactNormal.ToOrientationRotator().Quaternion();
				CursorToWorld->SetWorldLocationAndRotation(HitResult.Location, SurfaceRotation);
			}
		}
		else if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FHitResult TraceHitResult;
			PC->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();
			CursorToWorld->SetWorldLocation(TraceHitResult.Location);
			CursorToWorld->SetWorldRotation(CursorR);
		}
	}
}


void AMMemoryator::HandleMovementState()
{
	if (!HasAuthority())
	{
		return;
	}
	// TODO: Send this logic to custom Movement Component
	const auto Velocity = GetVelocity();

	const auto PlayerController = Cast<AMPlayerController>(GetController());
	if (!PlayerController)
		return;

	if (StateModelComponent->GetIsDashing()) return;

	// We consider movement as changing of either X or Y. Moving only along Z is falling.
	if (StateModelComponent->GetIsMoving() && FMath::IsNearlyZero(Velocity.X) && FMath::IsNearlyZero(Velocity.Y))
	{
		StateModelComponent->SetIsMoving(false);

		PlayerController->TurnSprintOff();
	}
	if (!StateModelComponent->GetIsMoving() && !(FMath::IsNearlyZero(Velocity.X) && FMath::IsNearlyZero(Velocity.Y)))
	{
		StateModelComponent->SetIsMoving(true);

		PlayerController->StartSprintTimer();
	}
}

void AMMemoryator::BeginPlay()
{
	Super::BeginPlay();

	if (InventoryComponent && InventoryComponent->GetSlots().IsEmpty())
	{
		InventoryComponent->Initialize(30, {{1, 8}, {1, 8}, {2, 8}, {2, 8}, {3, 8}, {3, 8}, {4, 8}, {4, 8}});
	}
}

void AMMemoryator::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (const auto MPlayerController = Cast<AMPlayerController>(Controller))
	{
		if (const auto Relation = RelationshipMap.Find(OtherActor->GetClass()); Relation && *Relation == ERelationType::Enemy)
		{
			AttackPuddleComponent->ActorsWithin.Add(*OtherActor->GetName(), OtherActor);
		}
	}
}

void AMMemoryator::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	AttackPuddleComponent->ActorsWithin.Remove(*OtherActor->GetName());
}
