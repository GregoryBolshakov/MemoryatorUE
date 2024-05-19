#include "MCharacter.h"

#include "Helpers/M2DRepresentationBlueprintLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/M2DRepresentationComponent.h"
#include "Components/MAttackPuddleComponent.h"
#include "Components/MBuffBarComponent.h"
#include "Components/MCommunicationComponent.h"
#include "Components/MIsActiveCheckerComponent.h"
#include "Components/MInventoryComponent.h"
#include "MMemoryator.h" // temporary include
#include "Components/CapsuleComponent.h"
#include "PaperSpriteComponent.h"
#include "Framework/MGameMode.h"
#include "Managers/MMetadataManager.h"
#include "Managers/SaveManager/MSaveManager.h"
#include "Managers/MWorldSaveTypes.h"
#include "Managers/MWorldGenerator.h"
#include "Net/UnrealNetwork.h"
#include "Components/MStateModelComponent.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"

AMCharacter::AMCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Health(MaxHealth)
{
	StateModelComponent = CreateDefaultSubobject<UMStateModelComponent>(TEXT("StateModel"));
	StateModelComponent->SetIsReplicated(true);

	InventoryComponent = CreateDefaultSubobject<UMInventoryComponent>(TEXT("InventoryrComponent"));

	CommunicationComponent = CreateDefaultSubobject<UMCommunicationComponent>(TEXT("CommunicationComponent"));

	IsActiveCheckerComponent = CreateDefaultSubobject<UMIsActiveCheckerComponent>(TEXT("IsActiveChecker"));
	IsActiveCheckerComponent->OnDisabledDelegate.BindUObject(this, &AMCharacter::OnDisabled);
	IsActiveCheckerComponent->OnEnabledDelegate.BindUObject(this, &AMCharacter::OnEnabled);

	FaceCameraComponent = CreateDefaultSubobject<UM2DRepresentationComponent>(TEXT("FaceCameraComponent"));
	FaceCameraComponent->SetupAttachment(RootComponent);

	BuffBarComponent = CreateDefaultSubobject<UMBuffBarComponent>(TEXT("BuffBar"));
	BuffBarComponent->SetupAttachment(FaceCameraComponent);
	BuffBarComponent->SetWidgetClass(BuffBarWidgetBPClass);

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

bool AMCharacter::Destroy(bool bNetForce, bool bShouldModifyLevel)
{
	if (const auto MetadataManager = AMGameMode::GetMetadataManager(this))
	{
		MetadataManager->Remove(FName(GetName()));
		return true;
	}
	return false;
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

void AMCharacter::InitialiseInventory(const TArray<FItem>& IN_Items) const
{
	InventoryComponent->Initialize(IN_Items.Num(), IN_Items);
}

void AMCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (StateModelComponent->GetIsDirty() && HasAuthority())
	{
		StateModelComponent->CleanDirty();
		StateModelReplicationTrigger = !StateModelReplicationTrigger;
		UpdateAnimation();
	}

	//TODO: Clean up the code below this. This is legacy logic related to M2DRepresentationComponent
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

	if (FaceCameraComponent)
	{
		FaceCameraComponent->SetMeshByGazeAndVelocity(GazeDirection, GetVelocity());
	}
}

void AMCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (FaceCameraComponent)
	{
		FaceCameraComponent->PostInitChildren();
	}
}

void AMCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMCharacter, StateModelReplicationTrigger);
}

void AMCharacter::OnRep_StateModelReplicationTrigger()
{
	UpdateAnimation();
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

float AMCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Damage)
	{
		if (BuffBarComponent)
		{
			if (BuffBarComponent->IsBuffSet(EBuffType::Invulnerability))
			{
				return 0.f;
			}

			const auto LaunchVelocity = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal() * 140.f; // TODO: add a UPROPERTY for the knock back length
			LaunchCharacter(LaunchVelocity, false, false);

			if (FaceCameraComponent)
			{
				FaceCameraComponent->SetColor(FLinearColor(1.f, 0.25f, 0.25f, 1.f));
			}

			StateModelComponent->SetIsTakingDamage(true);

			Health = FMath::Max(Health - abs(Damage), 0.f);
			if (FMath::IsNearlyZero(Health))
			{
				if (!GetClass()->IsChildOf(AMMemoryator::StaticClass())) // Temporary check
				{
					Destroy();
					return 0.f;
				}
			}

			BuffBarComponent->AddBuff(EBuffType::Invulnerability, 0.75f); // TODO: Add a perk for the duration
		}
	}
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void AMCharacter::BeginLoadFromSD(const FMCharacterSaveData& MCharacterSD)
{
	InitialiseInventory(MCharacterSD.InventoryContents);

	//Load house
	if (const auto WorldGenerator = AMGameMode::GetWorldGenerator(this))
	{
		if (const auto SaveManager = AMGameMode::GetSaveManager(this))
		{
			if (IsUidValid(MCharacterSD.HouseUid))
			{
				if (const auto HouseActor = Cast<AMOutpostHouse>(SaveManager->LoadMActorAndClearSD(MCharacterSD.HouseUid, WorldGenerator)))
				{
					HouseActor->MoveResidentIn(this);
				}
			}
		}
	}
}

void AMCharacter::OnEnabled_Implementation()
{
}

void AMCharacter::OnDisabled_Implementation()
{
}
