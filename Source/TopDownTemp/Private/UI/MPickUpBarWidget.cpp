#include "MPickUpBarWidget.h"

#include "Managers/MDropManager.h"
#include "UI/MInventorySlotWidget.h"
#include "UI/MInventoryWidget.h"
#include "StationaryActors/MPickableActor.h"
#include "Managers/MWorldGenerator.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/WrapBox.h"
#include "Framework/MGameMode.h"

void UMPickUpBarWidget::CreateSlots(TSet<UMInventoryComponent*>& InventoriesToRepresent)
{
	if (!pItemSlotsWrapBox)
		return;

	// Remove empty inventories from the list
	for (auto It = InventoriesToRepresent.CreateIterator(); It; ++It)
	{
		const auto Inventory = *It;
		if (Inventory->GetSlots().IsEmpty())
		{
			It.RemoveCurrent();
		}
	}

	// Empty the list of items to create them form scratch
	pItemSlotsWrapBox->ClearChildren();

	// Create slot widgets for all the items in the listed inventories
	for (const auto& InventoryComponent : InventoriesToRepresent)
	{
		UMInventoryWidget::CreateItemSlotWidgets(this, InventoryComponent, pItemSlotsWrapBox);
		if (const auto InventoryOwner = Cast<AMPickableActor>(InventoryComponent->GetOwner()))
		{
			for (auto& ItemSlot : InventoryComponent->GetSlots())
			{
				if (!ItemSlot.OnSlotChangedDelegate.IsBoundToObject(InventoryOwner))
				{
					ItemSlot.OnSlotChangedDelegate.BindDynamic(InventoryOwner, &AMPickableActor::OnItemChanged);
				}
				else
				{
					check(false);
				}
			}
		}
	}
}

void UMPickUpBarWidget::NativeDestruct()
{
	Super::NativeDestruct();

	if (const auto DropManager = AMGameMode::GetDropManager(this))
	{
		for (const auto& Inventory : DropManager->GetInventoriesToRepresent())
		{
			if (const auto InventoryOwner = Cast<AMPickableActor>(Inventory->GetOwner()))
			{
				for (auto& ItemSlot : Inventory->GetSlots())
				{
					if (!ItemSlot.OnSlotChangedDelegate.IsBoundToObject(InventoryOwner))
					{
						ItemSlot.OnSlotChangedDelegate.Unbind();
					}
				}
			}
		}
	}
}
