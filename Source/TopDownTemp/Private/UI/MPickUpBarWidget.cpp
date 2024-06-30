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

void UMPickUpBarWidget::CreateSlots(const TSet<UMInventoryComponent*>& InventoriesToRepresent)
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
		if (InventoryComponent->GetSlots().IsEmpty())
			continue;

		UMInventoryWidget::CreateItemSlotWidgets(this, InventoryComponent, pItemSlotsWrapBox);
		if (const auto InventoryOwner = Cast<AMPickableActor>(InventoryComponent->GetOwner()))
		{
			for (auto& ItemSlot : InventoryComponent->GetSlots())
			{
				ItemSlot.OnSlotChangedDelegate.RemoveDynamic(InventoryOwner, &AMPickableActor::OnItemChanged);
				ItemSlot.OnSlotChangedDelegate.AddDynamic(InventoryOwner, &AMPickableActor::OnItemChanged);
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
					ItemSlot.OnSlotChangedDelegate.RemoveDynamic(InventoryOwner, &AMPickableActor::OnItemChanged);
				}
			}
		}
	}
}
