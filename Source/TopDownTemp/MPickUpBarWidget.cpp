#include "MPickUpBarWidget.h"

#include "MDropManager.h"
#include "MInventorySlotWidget.h"
#include "MInventoryWidget.h"
#include "MPickableItem.h"
#include "MWorldGenerator.h"
#include "MWorldManager.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/WrapBox.h"

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
		if (const auto InventoryOwner = Cast<AMPickableItem>(InventoryComponent->GetOwner()))
		{
			for (auto& ItemSlot : InventoryComponent->GetSlots())
			{
				if (!ItemSlot.OnSlotChangedDelegate.IsBoundToObject(InventoryOwner))
				{
					ItemSlot.OnSlotChangedDelegate.BindDynamic(InventoryOwner, &AMPickableItem::OnItemChanged);
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

	if (const auto World = GetWorld())
	{
		if (const auto WorldManager = World->GetSubsystem<UMWorldManager>())
		{
			if (const auto WorldGenerator = WorldManager->GetWorldGenerator())
			{
				if (const auto DropManager = WorldGenerator->GetDropManager())
				{
					for (const auto& Inventory : DropManager->GetInventoriesToRepresent())
					{
						if (const auto InventoryOwner = Cast<AMPickableItem>(Inventory->GetOwner()))
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
		}
	}
}
