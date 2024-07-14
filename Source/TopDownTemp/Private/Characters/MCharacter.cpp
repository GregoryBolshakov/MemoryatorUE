#include "MCharacter.h"

#include "GameplayAbilities/Public/AbilitySystemComponent.h"
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
#include "Abilities/MGameplayAbility.h"
#include "Components/MAbilitySystemComponent.h"
#include "Framework/MGameMode.h"
#include "Managers/MMetadataManager.h"
#include "Managers/SaveManager/MSaveManager.h"
#include "Managers/MWorldSaveTypes.h"
#include "Managers/MWorldGenerator.h"
#include "Net/UnrealNetwork.h"
#include "Components/MStateModelComponent.h"
#include "Components/MStatsModelComponent.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"

AMCharacter::AMCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Optional because children might use different type for the model component
	StateModelComponent = CreateOptionalDefaultSubobject<UMStateModelComponent>(TEXT("StateModel"));
	StateModelComponent->SetIsReplicated(true);
	StateModelComponent->SetNetAddressable(); // Make DSO components net addressable

	StatsModelComponent = CreateOptionalDefaultSubobject<UMStatsModelComponent>(TEXT("StatsModel"));
	StatsModelComponent->SetIsReplicated(true);
	StatsModelComponent->SetNetAddressable();

	AbilitySystemComponent = CreateDefaultSubobject<UMAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetNetAddressable();

	InventoryComponent = CreateDefaultSubobject<UMInventoryComponent>(TEXT("InventoryrComponent"));
	InventoryComponent->SetIsReplicated(true);
	InventoryComponent->SetNetAddressable();

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

UAbilitySystemComponent* AMCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AMCharacter::InitialiseInventory(const TArray<FItem>& IN_Items)
{
	InventoryComponent->Initialize(IN_Items.Num(), IN_Items);
}

void AMCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (StateModelComponent->GetIsDirty() && HasAuthority())
	{
		StateModelComponent->CleanDirty();
		if (auto* SkeletalMesh = GetComponentByClass<USkeletalMeshComponent>())
		{
			if (auto* AnimInstance = SkeletalMesh->GetAnimInstance())
			{
				if (StateModelComponent->GetIsTurningRight())
				{
					auto test = 1;
				}
				AnimInstance->UpdateAnimation(0, false);
			}
		}
		UpdateAnimation();
	}
	if (StatsModelComponent->GetIsDirty() && HasAuthority())
	{
		StatsModelComponent->CleanDirty();
	}

	//TODO: Clean up the code below this. This is legacy logic related to M2DRepresentationComponent
	UpdateLastNonZeroDirection();

	auto GazeDirection = ForcedGazeVector.IsZero() ? LastNonZeroVelocity : ForcedGazeVector;
	GazeDirection.Z = 0;

	if (abs(UM2DRepresentationBlueprintLibrary::GetDeflectionAngle(GazeDirection, GetVelocity())) > 90.f)
	{
		StateModelComponent->SetIsReversing(true);
	}
	else
	{
		StateModelComponent->SetIsReversing(false);
	}

	if (FaceCameraComponent)
	{
		FaceCameraComponent->SetMeshByGazeAndVelocity(GazeDirection, GetVelocity());
	}
}

void AMCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (StateModelComponent && !HasAuthority())
	{
		StateModelComponent->OnDirtyDelegate.AddDynamic(this, &AMCharacter::UpdateAnimation);
	}
	if (FaceCameraComponent)
	{
		FaceCameraComponent->PostInitChildren();
	}
}

FMCharacterSaveData AMCharacter::GetSaveData() const
{
	// Start from the base and compose structs upwards
	FActorSaveData ActorSaveData = {
		GetClass(),
		GetActorLocation(),
		GetActorRotation(),
		AMGameMode::GetMetadataManager(this)->Find(FName(GetName()))->Uid,
		UMSaveManager::GetSaveDataForComponents(this)
	};
	FMCharacterSaveData MCharacterSD{
		ActorSaveData,
		GetSpeciesName(),
		GetStatsModelComponent()->GetHealth(),
	};
	// Save inventory if the AMCharacter has it
	if (InventoryComponent)
	{
		MCharacterSD.InventoryContents = InventoryComponent->GetItemCopies(false);
	}
	// Save house if the AMCharacter has it
	if (House)
	{
		if (const auto* HouseMetadata = AMGameMode::GetMetadataManager(this)->Find(FName(House->GetName())))
		{
			MCharacterSD.HouseUid = HouseMetadata->Uid;
		}
	}

	return MCharacterSD;
}

void AMCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMCharacter, Uid);
	DOREPLIFETIME(AMCharacter, LastNonZeroVelocity);
	DOREPLIFETIME(AMCharacter, ForcedGazeVector);
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
		AttackPuddleComponent->SetLength(StatsModelComponent->GetFightRangePlusRadius(GetRadius()));
		AttackPuddleComponent->SetAngle(StatsModelComponent->GetMeleeSpread());
	}
}

void AMCharacter::AddCharacterAbilities()
{
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent) || AbilitySystemComponent->bCharacterAbilitiesGiven)
	{
		return;
	}

	for (TSubclassOf<UMGameplayAbility>& StartupAbility : CharacterAbilities)
	{
		AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(StartupAbility, GetAbilityLevel(StartupAbility.GetDefaultObject()->AbilityID), static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputID), this));
	}

	AbilitySystemComponent->bCharacterAbilitiesGiven = true;
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

			StatsModelComponent->SetHealth(FMath::Max(StatsModelComponent->GetHealth() - abs(Damage), 0.f));
			if (FMath::IsNearlyZero(StatsModelComponent->GetHealth()))
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

void AMCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	AddCharacterAbilities();

	// ASC MixedMode replication requires that the ASC Owner's Owner be the Controller.
	SetOwner(NewController);
}

void AMCharacter::BeginLoadFromSD(const FMCharacterSaveData& MCharacterSD)
{
	InitialiseInventory(MCharacterSD.InventoryContents);

	//Load house
	if (const auto SaveManager = AMGameMode::GetSaveManager(this))
	{
		if (IsUidValid(MCharacterSD.HouseUid))
		{
			if (const auto HouseActor = Cast<AMOutpostHouse>(SaveManager->LoadMActorAndClearSD(MCharacterSD.HouseUid)))
			{
				HouseActor->MoveResidentIn(this);
			}
		}
	}
}

int32 AMCharacter::GetAbilityLevel(EMAbilityInputID AbilityID) const
{
	// TODO: Implement
	return 1;
}

void AMCharacter::OnEnabled_Implementation()
{
}

void AMCharacter::OnDisabled_Implementation()
{
}
