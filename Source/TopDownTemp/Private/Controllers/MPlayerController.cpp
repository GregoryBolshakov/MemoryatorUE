#include "MPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Components/MAttackPuddleComponent.h"
#include "Managers/MCommunicationManager.h"
#include "Characters/MMemoryator.h"
#include "Navigation/PathFollowingComponent.h"
#include "Managers/MConsoleCommandsManager.h"
#include "MInterfaceMobController.h"
#include "Camera/CameraComponent.h"
#include "Characters/MMob.h"
#include "Managers/MWorldManager.h"
#include "Managers/MWorldGenerator.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "StationaryActors/MActor.h"

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
			MovementComponent->MoveUpdatedComponent(
				(Value - LastDashProgressValue) * DashLength * MyCharacter->GetLastNonZeroVelocity().GetSafeNormal(),
				MyCharacter->GetActorRotation(), true);

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

	if (IsValid(GetPawn()))
	{
		ActiveSpringArm = Cast<
			USpringArmComponent>(GetPawn()->GetComponentByClass(USpringArmComponent::StaticClass()));
		ActiveCamera = Cast<UCameraComponent>(GetPawn()->GetComponentByClass(UCameraComponent::StaticClass()));
		ActiveCapsuleComponent = Cast<UCapsuleComponent>(
			GetPawn()->GetComponentByClass(UCapsuleComponent::StaticClass()));
	}
}

bool AMPlayerController::IsMovingByAI() const
{
	if (!IsValid(PathFollowingComponent))
	{
		return false;
	}

	return PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle;
}

void AMPlayerController::StopAIMovement()
{
	if (!IsValid(PathFollowingComponent))
	{
		return;
	}

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

void AMPlayerController::OnExperienceAdded(int Amount)
{
	if (const auto pCharacter = GetCharacter())
	{
		MakeFloatingNumber(pCharacter->GetActorLocation(), Amount, EFloatingNumberType::Experience);
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
	InputComponent->BindTouch(IE_Pressed, this, &AMPlayerController::MoveToTouchLocation);
	InputComponent->BindTouch(IE_Repeat, this, &AMPlayerController::MoveToTouchLocation);

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
	{
		return;
	}

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
		if (DistanceToActor + EnemyRadius <= MyCharacter.GetFightRangePlusMyRadius() * 3.5f &&
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
		MyCharacter.SetForcedGazeVector(VectorToEnemy);
		PuddleComponent->SetHiddenInGame(false);

		if (VectorToEnemy.Size2D() <= MyCharacter.GetFightRangePlusMyRadius() + ClosestEnemyRadius && !MyCharacter.
			GetIsFighting())
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
	{
		return;
	}

	if (const auto AttackPuddleComponent = MyCharacter->GetAttackPuddleComponent())
	{
		TArray<AActor*> OutActors;
		auto test0 = UEngineTypes::ConvertToObjectType(ECC_Pawn);
		UKismetSystemLibrary::BoxOverlapActors(GetWorld(), AttackPuddleComponent->GetComponentLocation(),
		                                       AttackPuddleComponent->Bounds.BoxExtent, {
			                                       UEngineTypes::ConvertToObjectType(
				                                       ECC_Pawn)
		                                       }, AMCharacter::StaticClass(), {MyCharacter}, OutActors);
		for (const auto Actor : OutActors)
		{
			if (!Actor)
			{
				continue;
			}

			if (const auto CapsuleComponent = Cast<UCapsuleComponent>(Actor->GetRootComponent()))
			{
				if (AttackPuddleComponent->IsCircleWithin(Actor->GetActorLocation(),
				                                          CapsuleComponent->GetScaledCapsuleRadius()))
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
		const float Distance = FVector::Dist(DestLocation, MyPawn->GetActorLocation());

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
	auto test0 = UEngineTypes::ConvertToObjectType(ECC_Pawn);
	if (GetHitResultUnderCursorForObjects({UEngineTypes::ConvertToObjectType(ECC_Pawn)}, true, HitResult))
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

void AMPlayerController::SyncOccludedActors()
{
	if (!ShouldCheckCameraOcclusion())
	{
		return;
	}

	FVector Start = ActiveCamera->GetComponentLocation();
	FVector End = GetPawn()->GetActorLocation();

	TArray<TEnumAsByte<EObjectTypeQuery>> CollisionObjectTypes;
	CollisionObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_OccludedTerrain));

	TArray<AActor*> ActorsToIgnore; // TODO: Add configuration to ignore actor types
	TArray<FHitResult> OutHits;

	auto ShouldDebug = DebugLineTraces ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	// Adjust the End so we don't occlude objects further the character
	End += (Start - End).GetSafeNormal() * ActiveCapsuleComponent->GetScaledCapsuleRadius() * (CapsulePercentageForTrace - 1.f);

	//TODO: Try using BoxTraceMulti as the horizontal shape will fit the screen size while checking occlusion.
	UKismetSystemLibrary::SphereTraceMulti(
	GetWorld(), Start, End, ActiveCapsuleComponent->GetScaledCapsuleRadius() * CapsulePercentageForTrace,
	UEngineTypes::ConvertToTraceType(ECC_OccludedTerrain), true, ActorsToIgnore, ShouldDebug, OutHits, true);

	// The list of actors hit by the line trace, that means that they are occluded from view
	TSet<const AActor*> ActorsJustOccluded;
	if (!OutHits.IsEmpty())
	{
		// Hide actors that are occluded by the camera
		for (FHitResult Hit : OutHits)
		{
			if (const AMActor* HitMActor = Cast<AMActor>(Hit.GetActor()))
			{
				HideOccludedActor(HitMActor, Hit.Distance);
				ActorsJustOccluded.Add(HitMActor);
			}
		}
	}
	TArray<FName> RemoveKeyList;
	// Show actors that are currently hidden but that are not occluded by the camera anymore
	for (auto& [Name, OccludedActor] : OccludedActors)
	{
		if (!ActorsJustOccluded.Contains(OccludedActor->MActor) && OccludedActor->TargetOpacity != 1.f)
		{
			ShowOccludedActor(OccludedActor);
		}
		if (OccludedActor->PendingKill) // Separate condition, because it can be removed only after the transition finishes
		{
			RemoveKeyList.Add(Name);
		}
	}

	for (const auto Name : RemoveKeyList)
		OccludedActors.Remove(Name);
}


void AMPlayerController::UpdateOpacity(UCameraOccludedActor* OccludedActor)
{
	if (!IsValid(OccludedActor))
		return;
	if (!IsValid(OccludedActor->MActor)) // Valid check
	{
		OccludedActors.Remove(FName(OccludedActor->Name));
		GetWorld()->GetTimerManager().ClearTimer(OccludedActor->OpacityTimerHandle);
		return;
	}

	OccludedActor->TransitionRemainTime -= GetWorld()->GetTimeSeconds() - OccludedActor->LastUpdateTime;
	OccludedActor->TransitionRemainTime = FMath::Clamp(OccludedActor->TransitionRemainTime, 0.f, OpacityTransitionDuration);
	OccludedActor->LastUpdateTime = GetWorld()->GetTimeSeconds();

	bool bAtLeastOneParamHasOpacity = false; // If object doesn't support opacity, remove its handler
	// Calculate New Opacity
	for (const auto& [StaticMesh, DynamicMaterialArrayWrapper] : OccludedActor->MActor->GetDynamicMaterials())
	{
		for (const auto DynamicMaterial : DynamicMaterialArrayWrapper.ArrayMaterialInstanceDynamic)
		{
			if (DynamicMaterial)
			{
				float CurrentOpacity;
				const auto bParamExist = DynamicMaterial->GetScalarParameterValue(FName("OccludedOpacity"), CurrentOpacity);
				if (!bParamExist)
					continue;

				bAtLeastOneParamHasOpacity = true;

				const auto InitialOpacity = OccludedActor->TargetOpacity == 1.f ? OccludedOpacity : 1.f;
				const float NewOpacity = FMath::Lerp(InitialOpacity, OccludedActor->TargetOpacity,
					1.f - OccludedActor->TransitionRemainTime / OpacityTransitionDuration);

				DynamicMaterial->SetScalarParameterValue("OccludedOpacity", NewOpacity);

				// Finished transition
				if (OccludedActor->TransitionRemainTime <= 0.f)
				{
					OccludedActor->TransitionRemainTime = 0.f;
					GetWorld()->GetTimerManager().ClearTimer(OccludedActor->OpacityTimerHandle);

					if (OccludedActor->TargetOpacity == 1.f)
					{
						OccludedActor->PendingKill = true; // Finished show transition. Finally can be removed.
					}
				}
			}
		}
	}

	if (!bAtLeastOneParamHasOpacity)
	{
		GetWorld()->GetTimerManager().ClearTimer(OccludedActor->OpacityTimerHandle);
	}
}

void AMPlayerController::HideOccludedActor(const AMActor* MActor, float Distance)
{
	const auto FoundResult = OccludedActors.Find(FName(MActor->GetName()));
	if (!FoundResult || !*FoundResult || !(*FoundResult)->MActor) // the ordinary case. Just marked to be hidden, initiate the transition 
	{
		UCameraOccludedActor* OccludedActor = NewObject<UCameraOccludedActor>(this);
		OccludedActor->Name = FName(MActor->GetName());
		OccludedActor->MActor = MActor;
		OccludedActor->Distance = Distance;
		OccludedActors.Add(FName(MActor->GetName()), OccludedActor);
		OnHideOccludedActor(OccludedActor);
	}
	else
	{
		if (!FMath::IsNearlyEqual((*FoundResult)->TargetOpacity, OccludedOpacity)) // Hide only if has been appearing to switch the transition
		{
			OnHideOccludedActor(*FoundResult);
		}
	}
}

void AMPlayerController::ShowOccludedActor(UCameraOccludedActor* OccludedActor)
{
	if (!OccludedActor->MActor)
	{
		return;
	}

	OnShowOccludedActor(OccludedActor);
}

void AMPlayerController::OnShowOccludedActor(UCameraOccludedActor* OccludedActor)
{
	OccludedActor->TargetOpacity = 1.f;
	OccludedActor->LastUpdateTime = GetWorld()->GetTimeSeconds();
	OccludedActor->TransitionRemainTime = FMath::Max(0.f, OpacityTransitionDuration - OccludedActor->TransitionRemainTime);

	GetWorld()->GetTimerManager().ClearTimer(OccludedActor->OpacityTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(OccludedActor->OpacityTimerHandle, [this, OccludedActor]() {
		UpdateOpacity(OccludedActor);
	}, 0.02f, true);
}

void AMPlayerController::OnHideOccludedActor(UCameraOccludedActor* OccludedActor)
{
	OccludedActor->TargetOpacity = OccludedOpacity;
	OccludedActor->LastUpdateTime = GetWorld()->GetTimeSeconds();
	OccludedActor->TransitionRemainTime = FMath::Max(0.f, OpacityTransitionDuration - OccludedActor->TransitionRemainTime);

	GetWorld()->GetTimerManager().ClearTimer(OccludedActor->OpacityTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(OccludedActor->OpacityTimerHandle, [this, OccludedActor]() {
		UpdateOpacity(OccludedActor);
	}, 0.02f, true);
}
