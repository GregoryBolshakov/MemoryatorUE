// Copyright Epic Games, Inc. All Rights Reserved.

#include "MCharacter.h"

#include "M2DRepresentationBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "M2DRepresentationComponent.h"
#include "MAttackPuddleComponent.h"
#include "MIsActiveCheckerComponent.h"
#include "MInventoryComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "PaperSpriteComponent.h"

AMCharacter::AMCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("CharacterMesh0")))
	, IsDying(false)
	, IsTakingDamage(false)
	, IsFighting(false)
	, IsMoving(false)
	, IsPicking(false)
	, MaxHealth(100.f)
	, Health(MaxHealth)
	, SightRange(500.f)
	, FightRange(50.f)
	, ForgetEnemyRange(1000.f)
	, Strength(10.f)
	, RetreatRange(200.f)
	, WalkSpeed(100.f)
	, RunSpeed(140.f)
	, bCanRetreat(true)
	, MeleeSpread(40.f)
{
	// Collection of sprites or flipbooks representing the character. It's not the Root Component!
	RepresentationComponent = CreateDefaultSubobject<UM2DRepresentationComponent>(TEXT("2DRepresentation"));
	RepresentationComponent->SetupAttachment(RootComponent);

	InventoryComponent = CreateDefaultSubobject<UMInventoryComponent>(TEXT("InventoryrComponent"));

	IsActiveCheckerComponent = CreateDefaultSubobject<UMIsActiveCheckerComponent>(TEXT("IsActiveChecker"));

	AttackPuddleComponent = CreateDefaultSubobject<UMAttackPuddleComponent>(TEXT("AttackPuddle"));
	AttackPuddleComponent->SetupAttachment(RootComponent);

	PerimeterOutlineComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PerimeterOutline"));
	PerimeterOutlineComponent->SetupAttachment(AttackPuddleComponent);
	AttackPuddleComponent->SetPerimeterOutline(PerimeterOutlineComponent);
#ifdef WITH_EDITOR
	PerimeterOutlineComponent->SetVisibleFlag(false);
#endif

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

	// Configure collision
	if (const auto PrimitiveRoot = Cast<UPrimitiveComponent>(RootComponent))
	{
		PrimitiveRoot->SetCollisionObjectType(ECC_Pawn);
		PrimitiveRoot->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		PrimitiveRoot->SetCollisionResponseToAllChannels(ECR_Ignore);
		PrimitiveRoot->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		PrimitiveRoot->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		PrimitiveRoot->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
		PrimitiveRoot->SetGenerateOverlapEvents(true);
	}

	//TODO: Make Z position constant. Now there is a performance loss due to floor collisions.
}

float AMCharacter::GetRadius() const
{
	if (const auto Capsule = GetCapsuleComponent())
	{
		return Capsule->GetScaledCapsuleRadius();
	}
	check(false);
	return 0.f;
}

void AMCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateLastNonZeroDirection();

	auto GazeDirection = ForcedGazeVector.IsZero() ? LastNonZeroVelocity : ForcedGazeVector;
	GazeDirection.Z = 0;

	if (abs(UM2DRepresentationBlueprintLibrary::GetDeflectionAngle(GazeDirection, GetVelocity())) > 90.f)
	{
		OnReverseMovementStartedDelegate.Broadcast();
	}
	else
	{
		OnReverseMovementStoppedDelegate.Broadcast();
	}

	RepresentationComponent->SetMeshByGazeAndVelocity(GazeDirection, GetVelocity());
}

void AMCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	RepresentationComponent->PostInitChildren();
}

void AMCharacter::UpdateLastNonZeroDirection()
{
	if (const auto CurrentVelocity = GetVelocity();
		!(CurrentVelocity.X == 0.f && CurrentVelocity.Y == 0.f))
	{
		LastNonZeroVelocity = GetVelocity();
	}
}

void AMCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AttackPuddleComponent)
	{
		AttackPuddleComponent->SetLength(GetFightRangePlusMyRadius());
		AttackPuddleComponent->SetAngle(MeleeSpread);
	}
}

float AMCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	if (Damage)
	{
		const auto LaunchVelocity = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal() * 140.f; // TODO: add a UPROPERTY for the knock back length
		LaunchCharacter(LaunchVelocity, false, false);

		RepresentationComponent->SetColor(FLinearColor(1.f, 0.2f, 0.2f, 1.f));

		IsTakingDamage = true;
		UpdateAnimation();
	}
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}