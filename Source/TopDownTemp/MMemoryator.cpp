// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMemoryator.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "MAttackPuddleComponent.h"
#include "MInventoryComponent.h"
#include "MPlayerController.h"
#include "PaperSpriteComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"

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
	CursorToWorld = CreateDefaultSubobject<UDecalComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("Material'/Game/Memoryator/Blueprints/M_Cursor_Decal.M_Cursor_Decal'"));
	if (DecalMaterialAsset.Succeeded())
	{
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	CursorToWorld->DecalSize = FVector(16.0f, 32.0f, 32.0f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());

	DirectionMarkerComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("DirectionMarker"));
	DirectionMarkerComponent->SetupAttachment(RootComponent);

	AttackPuddleComponent->PrimaryComponentTick.bCanEverTick = true;
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

	HandleAnimationStates();

	HandleCursor();
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


void AMMemoryator::HandleAnimationStates()
{
	// TODO: Send this logic to custom Movement Component
	const auto Velocity = GetVelocity();

	if (IsDashing) return;
	
	if (IsMoving && Velocity.IsZero())
	{
		IsMoving = false;
		UpdateAnimation();
	}
	if (!IsMoving && !Velocity.IsZero())
	{
		IsMoving = true;
		UpdateAnimation();
	}
}

void AMMemoryator::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	InventoryComponent->Initialize(40, {{0, 8}, {0, 8}, {1, 8}, {1, 8}, {2, 8}, {2, 8}, {3, 8}, {3, 8}});
}
