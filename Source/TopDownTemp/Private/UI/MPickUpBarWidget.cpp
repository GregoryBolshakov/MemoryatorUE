#include "MPickUpBarWidget.h"

#include "Managers/MDropManager.h"
#include "UI/MInventorySlotWidget.h"
#include "UI/MInventoryWidget.h"
#include "Components/Image.h"
#include "Components/WrapBox.h"

void UMPickUpBarWidget::CreateSlots(const TSet<const UMInventoryComponent*>& InventoriesToRepresent)
{
	if (!pItemSlotsWrapBox)
		return;

	// Empty the list of items to create them form scratch
	for ( int32 ChildIndex = pItemSlotsWrapBox->GetChildrenCount() - 1; ChildIndex >= 0; ChildIndex-- )
	{
		if (const auto InventorySlotWidget = Cast<UMInventorySlotWidget>(pItemSlotsWrapBox->GetChildAt(ChildIndex)))
		{
			if (InventorySlotWidget->IsDraggingAWidget())
			{
				InventorySlotWidget->SetVisibility(ESlateVisibility::Collapsed);
				continue; // TODO: Also swap with the first item to boost the loop
			}
		}
		pItemSlotsWrapBox->RemoveChildAt(ChildIndex);
	}

	// Create slot widgets for all the items in the listed inventories
	for (const auto& InventoryComponent : InventoriesToRepresent)
	{
		if (InventoryComponent->GetSlotsConst().IsEmpty())
			continue;

		UMInventoryWidget::CreateItemSlotWidgets(this, InventoryComponent, pItemSlotsWrapBox);
	}
}
