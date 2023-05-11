// Copyright Epic Games, Inc. All Rights Reserved.

#include "MPickableItem.h"

#include "M2DRepresentationComponent.h"
#include "MDropManager.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "MGameInstance.h"
#include "MMemoryator.h"
#include "MWorldGenerator.h"
#include "MWorldManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"

AMPickableItem::AMPickableItem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InventoryComponent = CreateDefaultSubobject<UMInventoryComponent>(TEXT("Inventory"));
}

void AMPickableItem::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Try to set up the collision of the capsule determining the collectable scope 
	if (const auto Capsule = Cast<UCapsuleComponent>(GetDefaultSubobjectByName(TEXT("CollectScopeCapsule"))))
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Capsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		Capsule->SetCollisionObjectType(EEC_Pickable);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Capsule->SetGenerateOverlapEvents(true);
	}

	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				if (pDropManager = pWorldGenerator->GetDropManager(); !pDropManager)
				{
					check(false);
				}
			}
		}
	}
}

void AMPickableItem::Initialise(const FItem& IN_Item)
{
	InventoryComponent->Initialize(1, {IN_Item});

	// Try to set sprite and quantity text using the data asset stored in the UMGameInstance
	if (const auto TaggedComponents = GetComponentsByTag(UPaperSpriteComponent::StaticClass(), "ItemSprite");
		!TaggedComponents.IsEmpty())
	{
		if (auto SpriteComponent = Cast<UPaperSpriteComponent>(TaggedComponents[0]))
		{
			if (const auto pGameInstace = GetGameInstance<UMGameInstance>();
				pGameInstace->ItemsDataAsset && pGameInstace->ItemsDataAsset->ItemsData.Num() > IN_Item.ID)
			{
				const auto ItemData = pGameInstace->ItemsDataAsset->ItemsData[IN_Item.ID];

				SpriteComponent->SetSprite(ItemData.IconSprite);
			}
		}
	}
	else
	{
		check(false);
	}

	if (const auto pWorld = GetWorld(); pDropManager)
	{
		if (const auto pPlayerPawn = UGameplayStatics::GetPlayerPawn(pWorld, 0))
		{
			if (const auto Capsule = Cast<UCapsuleComponent>(GetDefaultSubobjectByName(TEXT("CollectScopeCapsule"))))
			{
				auto Location2D = GetActorLocation();
				Location2D.Z = 0.f;
				auto PlayerLocation2D = pPlayerPawn->GetActorLocation();
				PlayerLocation2D.Z = 0.f;
				if (FVector::Distance(Location2D, PlayerLocation2D) < Capsule->GetScaledCapsuleRadius())
				{
					NotifyActorBeginOverlap(pPlayerPawn);
				}
			}
		}
	}
}

void AMPickableItem::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (const auto pWorld = GetWorld(); pDropManager)
	{
		if (const auto pPlayerPawn = UGameplayStatics::GetPlayerPawn(pWorld, 0);
			pPlayerPawn && pPlayerPawn == OtherActor)
		{
			pDropManager->AddInventory(InventoryComponent);
		}
	}
}

void AMPickableItem::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (const auto pWorld = GetWorld(); pDropManager)
	{
		if (const auto pPlayerPawn = UGameplayStatics::GetPlayerPawn(pWorld, 0);
			pPlayerPawn && pPlayerPawn == OtherActor)
		{
			pDropManager->RemoveInventory(InventoryComponent);
		}
	}
}

void AMPickableItem::OnItemChanged(int NewItemID, int NewQuantity)
{
	if (!pDropManager)
	{
		check(false);
		return;
	}

	bool IsEmpty = true;
	for (const auto [Item, OnChangedDelegate, IsLocked, IsSecret] : InventoryComponent->GetSlots())
	{
		if (Item.Quantity > 0)
		{
			IsEmpty = false;
			break;
		}
	}

	if (IsEmpty)
	{
		pDropManager->RemoveInventory(InventoryComponent);
		Destroy();
	}

	pDropManager->Update();
}
