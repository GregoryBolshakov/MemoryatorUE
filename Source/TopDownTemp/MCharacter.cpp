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
	 Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("CharacterMesh0")))
 	,SightRange(2000.f)
	,FightRange(50.f)
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

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	//TODO: Make Z position constant. Now there is a performance loss due to floor collisions.
}

void AMCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	HandleAnimationStates();

	UpdateLastNonZeroDirection();

	const auto GazeDirection = ForcedGazeVector.IsZero() ? LastNonZeroVelocity : ForcedGazeVector;
	const auto Angle = UM2DRepresentationComponent::GetCameraDeflectionAngle(this, GazeDirection);
	RepresentationComponent->SetMeshByRotation(Angle);
}

void AMCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	//TODO: there is usually IsActiveCheckerComponent->Disable();
	IsActiveCheckerComponent->SetUpCollisionPrimitive();
}

void AMCharacter::HandleAnimationStates()
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

void AMCharacter::UpdateLastNonZeroDirection()
{
	if (const auto CurrentVelocity = GetVelocity();
		!(CurrentVelocity.X == 0.f && CurrentVelocity.Y == 0.f))
	{
		LastNonZeroVelocity = GetVelocity();
	}
}
