#include "MDropManager.h"
#include "UI/MPickUpBarWidget.h"
#include "StationaryActors/MPickableActor.h"
#include "MWorldManager.h"
#include "MWorldGenerator.h"
#include "Characters/MCharacter.h"
#include "Components/MInventoryComponent.h"
#include "Framework/MGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NakamaManager/Private/ShopManagerClient.h"

UMDropManager::UMDropManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (const auto World = UObject::GetWorld())
	{
		if (const auto GameInstance = World->GetGameInstance<UMGameInstance>())
		{
			if (const auto NakamaManager = GameInstance->GetNakamaManager())
			{
				NakamaManager->ShopManagerClient->OnBundleTxnFinalizedDelegate.BindUObject(this, &UMDropManager::GiveBundleToPlayer);
			}
		}
	}
}

void UMDropManager::AddInventory(UMInventoryComponent* Inventory)
{
	if (Inventory->GetSlots().IsEmpty() || !GetWorld() || !GetWorld()->GetFirstPlayerController())
		return;

	if (InventoriesToRepresent.IsEmpty())
	{
		PickUpBarWidget = Cast<UMPickUpBarWidget>(CreateWidget(GetWorld()->GetFirstPlayerController(), PickUpBarWidgetBPClass));
		if (!PickUpBarWidget)
		{
			check(false);
			return;
		}
		PickUpBarWidget->AddToPlayerScreen();
	}

	InventoriesToRepresent.Add(Inventory);

	PickUpBarWidget->CreateSlots(InventoriesToRepresent);
}

void UMDropManager::RemoveInventory(UMInventoryComponent* Inventory)
{
	InventoriesToRepresent.Remove(Inventory);

	if (!PickUpBarWidget)
	{
		check(false);
		return;
	}

	if (!InventoriesToRepresent.IsEmpty())
	{
		PickUpBarWidget->CreateSlots(InventoriesToRepresent);
	}
	else
	{
		PickUpBarWidget->CloseWidget();
	}
}

void UMDropManager::Update()
{
	if (!InventoriesToRepresent.IsEmpty() && PickUpBarWidget)
	{
		PickUpBarWidget->CreateSlots(InventoriesToRepresent);
	}
}

void UMDropManager::SpawnPickableItem(const FItem& Item)
{
	check(Item.Quantity != 0);
	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				const auto pPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
				if (!pPlayer) { check(false); return; }

				if (const auto PickableItem = pWorldGenerator->SpawnActorInRadius<AMPickableActor>(AMPickableItemBPClass, pPlayer->GetActorLocation(), FRotator::ZeroRotator, {}, 25.f, 0.f))
				{
					PickableItem->InitialiseInventory({Item});
					return;
				}
			}
		}
	}
	check(false);
}

void UMDropManager::GiveBundleToPlayer(const FBundle& Bundle)
{
	if (const auto pPlayer = Cast<AMCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		if (const auto InventoryComponent = pPlayer->GetInventoryComponent())
		{
			for (const auto& Item : Bundle.items)
			{
				InventoryComponent->StoreItem({int(Item.itemid), Item.qty});
			}
		}
	}
}

TSubclassOf<UUserWidget> UMDropManager::gItemSlotWidgetBPClass = nullptr;

void UMDropManager::PostInitProperties()
{
	UObject::PostInitProperties();

	UMDropManager::gItemSlotWidgetBPClass = ItemSlotWidgetBPClass;
}