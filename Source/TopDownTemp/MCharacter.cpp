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

AMCharacter::AMCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("CharacterMesh0")))
	, IsDying(false)
	, IsTakingDamage(false)
	, IsFighting(false)
	, IsMoving(false)
	, IsPicking(false)
	, Health(100.f)
	, SightRange(2000.f)
	, FightRange(50.f)
	, Strength(10.f)
	, RetreatRange(200.f)
	, bCanRetreat(true)
{
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

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	//TODO: Make Z position constant. Now there is a performance loss due to floor collisions.
}

void AMCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateLastNonZeroDirection();

	const auto GazeDirection = ForcedGazeVector.IsZero() ? LastNonZeroVelocity : ForcedGazeVector;
	const auto Angle = UM2DRepresentationComponent::GetCameraDeflectionAngle(this, GazeDirection);
	RepresentationComponent->SetMeshByRotation(Angle);
}

void AMCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	RepresentationComponent->PostInitChildren();

	IsActiveCheckerComponent->DisableOwner();
	//IsActiveCheckerComponent->SetUpCollisionPrimitive();
}

void AMCharacter::UpdateLastNonZeroDirection()
{
	if (const auto CurrentVelocity = GetVelocity();
		!(CurrentVelocity.X == 0.f && CurrentVelocity.Y == 0.f))
	{
		LastNonZeroVelocity = GetVelocity();
	}
}
