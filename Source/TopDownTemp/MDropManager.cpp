// Copyright Epic Games, Inc. All Rights Reserved.

#include "MDropManager.h"
#include "MPickUpBarWidget.h"
#include "MPickableItem.h"
#include "MWorldManager.h"
#include "MWorldGenerator.h"
#include "MInventoryComponent.h"
#include "Kismet/GameplayStatics.h"

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

bool UMDropManager::OnDraggedItemDropped(const FItem& Item)
{
	check(Item.Quantity != 0);
	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				const auto pPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
				if (!pPlayer)
					return false;

				FActorSpawnParameters EmptySpawnParameters;
				if (const auto PickableItem = pWorldGenerator->SpawnActorInRadius<AMPickableItem>(AMPickableItemBPClass, pPlayer->GetActorLocation(), FRotator::ZeroRotator, EmptySpawnParameters, 25.f, 0.f))
				{
					PickableItem->Initialise(Item);
					return true;
				}
			}
		}
	}
	return false;
}