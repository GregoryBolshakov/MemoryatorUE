// Copyright Epic Games, Inc. All Rights Reserved.

#include "MPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "MCharacter.h"
#include "Navigation/PathFollowingComponent.h"

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

void AMPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// keep updating the destination every tick while desired
	if (bMoveToMouseCursor)
	{
		MoveToMouseCursor();
	}
}

void AMPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &AMPlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &AMPlayerController::OnSetDestinationReleased);

	// support touch devices 
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AMPlayerController::MoveToTouchLocation);
	InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AMPlayerController::MoveToTouchLocation);

	InputComponent->BindAxis("MoveForward", this, &AMPlayerController::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AMPlayerController::MoveRight);
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
	const FRotator PitchRotation = FRotator(0, GetControlRotation().Pitch, 0);

	AMCharacter* MyPawn = Cast<AMCharacter>(GetPawn());
	if (IsValid(MyPawn))
	{
		MyPawn->AddMovementInput(FRotationMatrix(PitchRotation).GetUnitAxis(EAxis::X), Value);
	}
}

void AMPlayerController::MoveToMouseCursor()
{
	APawn* const MyPawn = GetPawn();
	// We don't start AI route while player is moving by manual controls
	if (!MyPawn->GetVelocity().IsZero() && !IsMovingByAI())
	{
		return;
	}

	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		if (AMCharacter* MyCharacter = Cast<AMCharacter>(MyPawn))
		{
			if (MyCharacter->GetCursorToWorld())
			{
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, MyCharacter->GetCursorToWorld()->GetComponentLocation());
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
	if (!MyPawn->GetVelocity().IsZero() && !IsMovingByAI())
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

void AMPlayerController::OnSetDestinationPressed()
{
	// set flag to keep updating destination until released
	bMoveToMouseCursor = true;
}

void AMPlayerController::OnSetDestinationReleased()
{
	// clear flag to indicate we should stop updating the destination
	bMoveToMouseCursor = false;
}
