// Copyright Epic Games, Inc. All Rights Reserved.

#include "MCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "M2DRepresentationComponent.h"
#include "MIsActiveCheckerComponent.h"
#include "MPlayerController.h"
#include "PaperSpriteComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"

AMCharacter::AMCharacter(const FObjectInitializer& ObjectInitializer) :
	Super(
		ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("CharacterMesh0"))
		 )
{
	const auto CollisionPrimitive = Cast<UPrimitiveComponent>(RootComponent);
	check(CollisionPrimitive);
	CollisionPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionPrimitive->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionPrimitive->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	CollisionPrimitive->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionPrimitive->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	CollisionPrimitive->SetGenerateOverlapEvents(false);

	// Collection of sprites or flipbooks representing the character. It's not the Root Component!
	RepresentationComponent = CreateDefaultSubobject<UM2DRepresentationComponent>(TEXT("2DRepresentation"));
	RepresentationComponent->SetupAttachment(RootComponent);

	IsActiveCheckerComponent = CreateDefaultSubobject<UMIsActiveCheckerComponent>(TEXT("IsActiveChecker"));

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

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

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	//TODO: Make Z position constant. Now there is a performance loss due to floor collisions.
}

void AMCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	HandleCursor();

	HandleAnimations();
}

void AMCharacter::AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce)
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

void AMCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	//TODO: there is usually IsActiveCheckerComponent->Disable();
	IsActiveCheckerComponent->SetUpCollisionPrimitive();
}

void AMCharacter::HandleCursor() const
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

void AMCharacter::HandleAnimations() 
{
	// TODO: Send this logic to custom Movement Component
	const auto Velocity = GetVelocity();

	if (IsMoving && Velocity == FVector::ZeroVector)
	{
		IsMoving = false;
		UpdateAnimation();
	}
	if (!IsMoving && Velocity != FVector::ZeroVector)
	{
		IsMoving = true;
		UpdateAnimation();
	}
}
