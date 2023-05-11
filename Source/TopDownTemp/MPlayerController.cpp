// Copyright Epic Games, Inc. All Rights Reserved.

#include "MPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "MAttackPuddleComponent.h"
#include "MCommunicationManager.h"
#include "MMemoryator.h"
#include "Navigation/PathFollowingComponent.h"
#include "MConsoleCommandsManager.h"
#include "MInterfaceMobController.h"
#include "MMob.h"
#include "MWorldManager.h"
#include "MWorldGenerator.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AMPlayerController::AMPlayerController(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	bMoveToMouseCursor(0),
	PathFollowingComponent(nullptr)
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	PathFollowingComponent = FindComponentByClass<UPathFollowingComponent>();
	if (PathFollowingComponent == nullptr)
	{
		PathFollowingComponent = CreateDefaultSubobject<UPathFollowingComponent>(TEXT("PathFollowingComponent"));
		PathFollowingComponent->Initialize();
	}

	ConsoleCommandsManager = CreateDefaultSubobject<UMConsoleCommandsManager>(TEXT("ConsoleCommandsManager"));
}

FTimerHandle tempTimer;
void AMPlayerController::TimelineProgress(float Value)
{
	if (const auto MyCharacter = Cast<AMCharacter>(GetPawn()))
	{
		if (FMath::IsNearlyEqual(Value, 1.f, 0.0001f)) // Passed the timeline, stop dashing
		{
			EnableInput(this);
			MyCharacter->SetIsDashing(false);
			MyCharacter->UpdateAnimation();
			DashVelocityTimeline.Stop();

			TurnSprintOff();

			return;
		}

		if (const auto MovementComponent = Cast<UCharacterMovementComponent>(MyCharacter->GetMovementComponent()))
		{
			// Set velocity to the distance we should have passed since the last frame and force movement component to consume the velocity
			MovementComponent->UpdateComponentVelocity();
			MovementComponent->MoveUpdatedComponent((Value - LastDashProgressValue) * DashLength * MyCharacter->GetLastNonZeroVelocity().GetSafeNormal(), MyCharacter->GetActorRotation(), true);

			// Update last frame info
			TimeSinceLastDashUpdate = FDateTime::UtcNow();
			LastDashProgressValue = Value;
		}
	}
}

void AMPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (DashVelocityCurve)
	{
		FOnTimelineFloat TimelineProgress;
		TimelineProgress.BindDynamic(this, &AMPlayerController::TimelineProgress);
		DashVelocityTimeline.AddInterpFloat(DashVelocityCurve, TimelineProgress);
		DashVelocityTimeline.SetLooping(false);
	}
}

bool AMPlayerController::IsMovingByAI() const
{
	if (!IsValid(PathFollowingComponent))
		return false;

	return PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle;
}

void AMPlayerController::StopAIMovement()
{
	if (!IsValid(PathFollowingComponent))
		return;

	PathFollowingComponent->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished);
}

FTimerHandle RunningTimerHandle;
void AMPlayerController::StartSprintTimer()
{
	if (const auto MyCharacter = Cast<AMCharacter>(GetCharacter()))
	{
		if (auto& TimerManager = GetWorld()->GetTimerManager();
			!TimerManager.IsTimerActive(RunningTimerHandle))
		{
			TimerManager.SetTimer(RunningTimerHandle, [this, &MyCharacter]
			{
				TurnSprintOn();
			}, MyCharacter->GetTimeBeforeSprint(), false);
		}
	}
}

void AMPlayerController::TurnSprintOn()
{
	if (const auto MyCharacter = Cast<AMCharacter>(GetCharacter()); MyCharacter && !MyCharacter->GetIsSprinting())
	{
		MyCharacter->SetIsSprinting(true);
		if (const auto MovementComponent = MyCharacter->GetCharacterMovement())
		{
			MovementComponent->MaxWalkSpeed = MyCharacter->GetSprintSpeed();
		}
	}
}

void AMPlayerController::TurnSprintOff()
{
	GetWorld()->GetTimerManager().ClearTimer(RunningTimerHandle);
	if (const auto MyCharacter = Cast<AMCharacter>(GetCharacter()); MyCharacter && MyCharacter->GetIsSprinting())
	{
		MyCharacter->SetIsSprinting(false);
		if (const auto CharacterMovement = MyCharacter->GetCharacterMovement())
		{
			CharacterMovement->MaxWalkSpeed = MyCharacter->GetWalkSpeed();
		}
	}
}

void AMPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	DashVelocityTimeline.TickTimeline(DeltaTime);

	const auto pWorld = GetWorld();
	if (!pWorld)
	{
		check(false);
		return;
	}

	const auto pMyCharacter = Cast<AMCharacter>(GetPawn());
	if (!pMyCharacter)
	{
		check(false);
		return;
	}

	// Get actors nearby every N seconds. We don't need to do this every frame
	if (auto& TimerManager = GetWorld()->GetTimerManager();
		!TimerManager.IsTimerActive(ActorsNearbyUpdateTimerHandle))
	{
		TimerManager.SetTimer(ActorsNearbyUpdateTimerHandle, [this, pWorld, pMyCharacter]
		{
			SetDynamicActorsNearby(*pWorld, *pMyCharacter);
		}, 1.f, false);
	}

	UpdateClosestEnemy(*pMyCharacter);

	// Keep updating the destination every tick while desired
	if (bMoveToMouseCursor)
	{
		MoveToMouseCursor();
	}
}

void AMPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	//InputComponent->BindAction("SetDestination", IE_Pressed, this, &AMPlayerController::OnSetDestinationPressed);
	//InputComponent->BindAction("SetDestination", IE_Released, this, &AMPlayerController::OnSetDestinationReleased);

	InputComponent->BindAction("ToggleIsTurningAround", IE_Pressed, this,
	                           &AMPlayerController::OnToggleTurnAroundPressed);
	InputComponent->BindAction("ToggleIsTurningAround", IE_Released, this,
	                           &AMPlayerController::OnToggleTurnAroundReleased);

	//InputComponent->BindAction("ToggleIsFighting", IE_Pressed, this, &AMPlayerController::OnToggleFightPressed);

	// we use only pressed, because player cannot stop performing the dash by himself
	InputComponent->BindAction("Dash", IE_Pressed, this, &AMPlayerController::OnDashPressed);

	InputComponent->BindAction("LeftMouseClick", IE_Released, this, &AMPlayerController::OnLeftMouseClick);

	// support touch devices 
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AMPlayerController::MoveToTouchLocation);
	InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AMPlayerController::MoveToTouchLocation);

	InputComponent->BindAxis("MoveForward", this, &AMPlayerController::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AMPlayerController::MoveRight);

	InputComponent->BindAxis("TurnAround", this, &AMPlayerController::TurnAround);
}

void AMPlayerController::SetDynamicActorsNearby(const UWorld& World, AMCharacter& MyCharacter)
{
	if (const auto pWorldGenerator = World.GetSubsystem<UMWorldManager>()->GetWorldGenerator())
	{
		const auto CharacterLocation = MyCharacter.GetTransform().GetLocation();
		const auto ForgetEnemyRange = MyCharacter.GetForgetEnemyRange();
		const auto DynamicActorsNearby = pWorldGenerator->GetActorsInRect(
			CharacterLocation - FVector(ForgetEnemyRange, ForgetEnemyRange, 0.f),
			CharacterLocation + FVector(ForgetEnemyRange, ForgetEnemyRange, 0.f), true);
		EnemiesNearby.Empty();

		if (!DynamicActorsNearby.IsEmpty())
		{
			for (const auto& [Name, DynamicActor] : DynamicActorsNearby)
			{
				// The actors are taken in a square area, in the corners the distance is greater than the radius
				const auto DistanceToActor = FVector::Distance(DynamicActor->GetTransform().GetLocation(),
				                                               MyCharacter.GetTransform().GetLocation());
				if (DistanceToActor <= MyCharacter.GetForgetEnemyRange())
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

void AMPlayerController::UpdateClosestEnemy(AMCharacter& MyCharacter)
{
	const auto PuddleComponent = MyCharacter.GetAttackPuddleComponent();
	if (!PuddleComponent)
		return;

	if (MyCharacter.GetIsDashing()) // check for any action that shouldn't rotate character towards enemy
	{
		PuddleComponent->SetHiddenInGame(true);
		return;
	}

	const auto CharacterLocation = MyCharacter.GetTransform().GetLocation();
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
		if (DistanceToActor + EnemyRadius <= MyCharacter.GetFightRangePlusMyRadius() * 3.5f && // Actor is within sight range TODO: put the " * 2.5f" to the properties
			(!ClosestEnemy || DistanceToActor < FVector::Distance(CharacterLocation, ClosestEnemy->GetActorLocation()))) // It is either the only one in sight or the closest
		{
			ClosestEnemy = EnemyActor;
			ClosestEnemyRadius = EnemyRadius;
		}
	}

	if (ClosestEnemy)
	{
		const auto VectorToEnemy = ClosestEnemy->GetActorLocation() - CharacterLocation;
		MyCharacter.SetForcedGazeVector(VectorToEnemy);
		PuddleComponent->SetHiddenInGame(false);

		if (VectorToEnemy.Size2D() <= MyCharacter.GetFightRangePlusMyRadius() + ClosestEnemyRadius && !MyCharacter.GetIsFighting())
		{
			MyCharacter.SetIsFighting(true);
		}
	}
	else
	{
		MyCharacter.SetForcedGazeVector(FVector::ZeroVector);
		PuddleComponent->SetHiddenInGame(true);
		if (bEnemyWasValid) // Was valid but no there is no such
		{
			StartSprintTimer();
		}
	}
}

void AMPlayerController::OnHit()
{
	const auto MyCharacter = Cast<AMCharacter>(GetCharacter());
	if (!MyCharacter)
		return;

	if (const auto AttackPuddleComponent = MyCharacter->GetAttackPuddleComponent())
	{
		TArray<AActor*> OutActors;
		auto test0 = UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn);
		UKismetSystemLibrary::BoxOverlapActors(GetWorld(), AttackPuddleComponent->GetComponentLocation(), AttackPuddleComponent->Bounds.BoxExtent, {UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn)}, AMCharacter::StaticClass(), {MyCharacter}, OutActors);
		for (const auto Actor : OutActors)
		{
			if (!Actor)
				continue;

			if (const auto CapsuleComponent = Cast<UCapsuleComponent>(Actor->GetRootComponent()))
			{
				if (AttackPuddleComponent->IsCircleWithin(Actor->GetActorLocation(), CapsuleComponent->GetScaledCapsuleRadius()))
				{
					Actor->TakeDamage(MyCharacter->GetStrength(), {}, this, MyCharacter);
				}
			}
		}
	}
}

void AMPlayerController::MoveRight(float Value)
{
	const FRotator YawRotation = FRotator(0, GetControlRotation().Yaw, 0);

	AMCharacter* MyPawn = Cast<AMCharacter>(GetPawn());
	if (IsValid(MyPawn))
	{
		MyPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
	}
}

void AMPlayerController::MoveForward(float Value)
{
	const FRotator YawRotation = FRotator(0, GetControlRotation().Yaw, 0);

	AMCharacter* MyPawn = Cast<AMCharacter>(GetPawn());
	if (IsValid(MyPawn))
	{
		MyPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
	}
}

void AMPlayerController::TurnAround(float Value)
{
	if (bIsTurningAround)
	{
		AMCharacter* MyPawn = Cast<AMCharacter>(GetPawn());
		if (IsValid(MyPawn))
		{
			const FRotator Rotation(0.0f, Value, 0.0f);

			AddYawInput(Rotation.Yaw); // Rotate Controller, to change the direction of the Pawn movement
			MyPawn->SetActorRotation(GetControlRotation()); // Pawn visual rotation only
		}
	}
}

void AMPlayerController::MoveToMouseCursor()
{
	APawn* const MyPawn = GetPawn();
	// We don't start AI route while player is moving by manual controls
	if ((MyPawn->GetVelocity().Size() > 0 || MyPawn->GetPendingMovementInputVector().Size() > 0) && !IsMovingByAI())
	{
		return;
	}

	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		if (auto* MyCharacter = Cast<AMMemoryator>(MyPawn))
		{
			if (MyCharacter->GetCursorToWorld())
			{
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(
					this, MyCharacter->GetCursorToWorld()->GetComponentLocation());
			}
		}
	}
	else
	{
		// Trace to see what is under the mouse cursor
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, false, Hit);

		if (Hit.bBlockingHit)
		{
			// We hit something, move there
			SetNewMoveDestination(Hit.ImpactPoint);
		}
	}
}

void AMPlayerController::MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	APawn* const MyPawn = GetPawn();
	// We don't start AI route while player is moving by manual controls
	if ((MyPawn->GetVelocity().Size() > 0 || MyPawn->GetPendingMovementInputVector().Size() > 0) && !IsMovingByAI())
	{
		return;
	}

	FVector2D ScreenSpaceLocation(Location);

	// Trace to see what is under the touch location
	FHitResult HitResult;
	GetHitResultAtScreenPosition(ScreenSpaceLocation, CurrentClickTraceChannel, true, HitResult);
	if (HitResult.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(HitResult.ImpactPoint);
	}
}

void AMPlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	APawn* const MyPawn = GetPawn();
	if (MyPawn)
	{
		float const Distance = FVector::Dist(DestLocation, MyPawn->GetActorLocation());

		// We need to issue move command only if far enough in order for walk animation to play correctly
		if ((Distance > 120.0f))
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, DestLocation);
		}
	}
}

/*void AMPlayerController::OnSetDestinationPressed()
{
	// set flag to keep updating destination until released
	bMoveToMouseCursor = true;
}

void AMPlayerController::OnSetDestinationReleased()
{
	// clear flag to indicate we should stop updating the destination
	bMoveToMouseCursor = false;
}*/

void AMPlayerController::OnToggleTurnAroundPressed()
{
	bIsTurningAround = true;
}

void AMPlayerController::OnToggleTurnAroundReleased()
{
	bIsTurningAround = false;
}

void AMPlayerController::OnToggleFightPressed()
{
	if (const auto MyCharacter = Cast<AMCharacter>(GetPawn()); MyCharacter && !MyCharacter->GetIsFighting())
	{
		MyCharacter->SetIsFighting(true);
	}
}

void AMPlayerController::OnLeftMouseClick()
{
	FHitResult HitResult;
	auto test0 = UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn);
	if (GetHitResultUnderCursorForObjects({UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn)}, true, HitResult))
	{
		AMMob* ClickedMob = Cast<AMMob>(HitResult.GetActor());
		if (ClickedMob)
		{
			if (const auto WorldManager = GetWorld()->GetSubsystem<UMWorldManager>())
			{
				if (const auto WorldGenerator = WorldManager->GetWorldGenerator())
				{
					if (const auto CommunicationManager = WorldGenerator->GetCommunicationManager())
					{
						CommunicationManager->SpeakTo(ClickedMob);
					}
				}
			}
		}
	}
}

void AMPlayerController::OnDashPressed()
{
	if (const auto MyCharacter = Cast<AMCharacter>(GetPawn()); MyCharacter && !MyCharacter->GetIsDashing())
	{
		MyCharacter->SetForcedGazeVector(FVector::ZeroVector); // If character was facing enemy, stop
		MyCharacter->SetIsDashing(true);
		MyCharacter->SetIsMoving(false);
		MyCharacter->SetIsFighting(false);
		MyCharacter->UpdateAnimation();
		DisableInput(this);

		TimeSinceLastDashUpdate = FDateTime::UtcNow();
		LastDashProgressValue = 0.f;

		DashVelocityTimeline.PlayFromStart();
	}
}

bool AMPlayerController::ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor)
{
	bool bHandled = Super::ProcessConsoleExec(Cmd, Ar, Executor);

	if (!bHandled && ConsoleCommandsManager != nullptr)
	{
		bHandled |= ConsoleCommandsManager->ProcessConsoleExec(Cmd, Ar, Executor);
	}

	return bHandled;
}
