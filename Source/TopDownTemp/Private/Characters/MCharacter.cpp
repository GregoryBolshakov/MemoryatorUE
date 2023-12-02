#include "MCharacter.h"

#include "Helpers/M2DRepresentationBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/M2DRepresentationComponent.h"
#include "Components/MAttackPuddleComponent.h"
#include "Components/MBuffBarComponent.h"
#include "Components/MCommunicationComponent.h"
#include "Framework/MGameInstance.h"
#include "Components/MIsActiveCheckerComponent.h"
#include "Components/MInventoryComponent.h"
#include "MMemoryator.h" // temporary include
#include "Components/CapsuleComponent.h"
#include "PaperSpriteComponent.h"
#include "Managers/MWorldGenerator.h"
#include "Managers/MWorldManager.h"

AMCharacter::AMCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("CharacterMesh0")))
	, Health(MaxHealth)
{
	// Collection of sprites or flipbooks representing the character. It's not the Root Component!
	RepresentationComponent = CreateDefaultSubobject<UM2DRepresentationComponent>(TEXT("2DRepresentation"));
	RepresentationComponent->SetupAttachment(RootComponent);

	InventoryComponent = CreateDefaultSubobject<UMInventoryComponent>(TEXT("InventoryrComponent"));

	CommunicationComponent = CreateDefaultSubobject<UMCommunicationComponent>(TEXT("CommunicationComponent"));

	IsActiveCheckerComponent = CreateDefaultSubobject<UMIsActiveCheckerComponent>(TEXT("IsActiveChecker"));
	IsActiveCheckerComponent->OnDisabledDelegate.BindUObject(this, &AMCharacter::OnDisabled);
	IsActiveCheckerComponent->OnEnabledDelegate.BindUObject(this, &AMCharacter::OnEnabled);

	BuffBarComponent = CreateDefaultSubobject<UMBuffBarComponent>(TEXT("BuffBar"));
	BuffBarComponent->SetupAttachment(RepresentationComponent);
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
	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				pWorldGenerator->RemoveActorFromGrid(this);
				return true;
			}
		}
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

	if (const auto pGameInstance = GetGameInstance<UMGameInstance>())
	{
		if (const auto CharacterSpeciesDataAsset = pGameInstance->CharacterSpeciesDataAsset)
		{
			// Temporary solution. We will do this only when spawn a new character. Already existing will load the SpeciesName from save
			const auto AllPossibleSpecies = CharacterSpeciesDataAsset->GetAllNamesByClass(GetClass());
			if (!AllPossibleSpecies.IsEmpty())
			{
				SpeciesName = AllPossibleSpecies[FMath::RandRange(0, AllPossibleSpecies.Num()-1)];
			}
		}

		// Generate starting inventory
		if (InventoryComponent)
		{
			if (const auto pCharacterSpeciesDataAsset = pGameInstance->CharacterSpeciesDataAsset)
			{
				if (const auto MyData = pCharacterSpeciesDataAsset->Data.Find(SpeciesName))
				{
					TArray<FItem> StartingInventory;
					for (const auto [ItemID, ItemData] : MyData->StartingItemsData)
					{
						if (FMath::RandRange(0.f, 1.f) <= ItemData.Probability)
						{
							StartingInventory.Add({ItemID, FMath::RandRange(ItemData.MinMaxStartQuantity.X, ItemData.MinMaxStartQuantity.Y)});
						}
					}
					InventoryComponent->Initialize(StartingInventory.Num(), StartingInventory);
				}
			}
			InventoryComponent->SetFlagToAllSlots(FSlot::ESlotFlags::Secret);
			InventoryComponent->SetFlagToAllSlots(FSlot::ESlotFlags::Locked);
		}
	}
}

float AMCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
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

			RepresentationComponent->SetColor(FLinearColor(1.f, 0.25f, 0.25f, 1.f));

			IsTakingDamage = true;
			UpdateAnimation();

			Health = FMath::Max(Health - abs(Damage), 0.f);
			if (FMath::IsNearlyZero(Health))
			{
				if (!GetClass()->IsChildOf(AMMemoryator::StaticClass())) // Temporary check
				{
					//Destroy(); TODO: Fix this
					return 0.f;
				}
			}

			BuffBarComponent->AddBuff(EBuffType::Invulnerability, 0.75f); // TODO: Add a perk for the duration
		}
	}
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void AMCharacter::OnEnabled_Implementation()
{
}

void AMCharacter::OnDisabled_Implementation()
{
}
