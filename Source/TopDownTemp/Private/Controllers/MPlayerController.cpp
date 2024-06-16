#include "MPlayerController.h"

#include "AbilitySystemComponent.h"
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
#include "Components/MStateModelComponent.h"
#include "Managers/MWorldGenerator.h"
#include "Components/CapsuleComponent.h"
#include "Components/MStatsModelComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "Framework/MGameMode.h"
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

void AMPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	AMCharacter* MCharacter = Cast<AMCharacter>(P);
	if (MCharacter)
	{
		MCharacter->GetAbilitySystemComponent()->InitAbilityActorInfo(MCharacter, MCharacter);
	}
}

void AMPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (DeferredPawnToPossess)
	{
		Possess(DeferredPawnToPossess);
		DeferredPawnToPossess = nullptr;
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
	if (const auto MyCharacter = Cast<AMCharacter>(GetPawn()))
	{
		if (auto& TimerManager = GetWorld()->GetTimerManager();
			!TimerManager.IsTimerActive(RunningTimerHandle))
		{
			TimerManager.SetTimer(RunningTimerHandle, [this, &MyCharacter]
			{
				TurnSprintOn();
			}, MyCharacter->GetStatsModelComponent()->GetTimeBeforeSprint(), false);
		}
	}
}

void AMPlayerController::TurnSprintOn()
{
	if (const auto MyCharacter = Cast<AMCharacter>(GetPawn()); MyCharacter && !MyCharacter->GetStateModelComponent()->GetIsSprinting())
	{
		MyCharacter->GetStateModelComponent()->SetIsSprinting(true);
		if (const auto MovementComponent = MyCharacter->GetCharacterMovement())
		{
			MovementComponent->MaxWalkSpeed = MyCharacter->GetStatsModelComponent()->GetSprintSpeed();
		}
	}
}

void AMPlayerController::TurnSprintOff()
{
	GetWorld()->GetTimerManager().ClearTimer(RunningTimerHandle);
	if (const auto MyCharacter = Cast<AMCharacter>(GetPawn()); MyCharacter && MyCharacter->GetStateModelComponent()->GetIsSprinting())
	{
		MyCharacter->GetStateModelComponent()->SetIsSprinting(false);
		if (const auto CharacterMovement = MyCharacter->GetCharacterMovement())
		{
			CharacterMovement->MaxWalkSpeed = MyCharacter->GetStatsModelComponent()->GetWalkSpeed();
		}
	}
}

void AMPlayerController::OnExperienceAdded(int Amount)
{
	if (const auto pCharacter = GetPawn())
	{
		MakeFloatingNumber(pCharacter->GetActorLocation(), Amount, EFloatingNumberType::Experience);
	}
}

void AMPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Keep updating the destination every tick while desired
	// TODO: support this if needed, right now it doesn't work
	/*if (bMoveToMouseCursor)
	{
		MoveToMouseCursor();
	}*/
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

	// Outdated implementation TODO: Delete when finish Gameplay Ability implementation 
	// we use only pressed, because player cannot stop performing the dash by himself
	//InputComponent->BindAction("Dash", IE_Pressed, this, &AMPlayerController::OnDashPressed);

	InputComponent->BindAction("LeftMouseClick", IE_Released, this, &AMPlayerController::OnLeftMouseClick);

	// support touch devices 
	InputComponent->BindTouch(IE_Pressed, this, &AMPlayerController::MoveToTouchLocation);
	InputComponent->BindTouch(IE_Repeat, this, &AMPlayerController::MoveToTouchLocation);

	InputComponent->BindAxis("MoveForward", this, &AMPlayerController::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AMPlayerController::MoveRight);

	InputComponent->BindAxis("TurnAround", this, &AMPlayerController::TurnAround);
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
	if (const auto MyCharacter = Cast<AMCharacter>(GetPawn()); MyCharacter && !MyCharacter->GetStateModelComponent()->GetIsFighting())
	{
		MyCharacter->GetStateModelComponent()->SetIsFighting(true);
	}
}

void AMPlayerController::OnLeftMouseClick()
{
	FHitResult HitResult;
	if (GetHitResultUnderCursorForObjects({UEngineTypes::ConvertToObjectType(ECC_Pawn)}, true, HitResult))
	{
		AMMob* ClickedMob = Cast<AMMob>(HitResult.GetActor());
		if (ClickedMob)
		{
			if (const auto CommunicationManager = AMGameMode::GetCommunicationManager(this))
			{
				CommunicationManager->SpeakTo(ClickedMob);
			}
		}
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
