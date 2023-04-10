#include "MPickUpBarWidget.h"

#include "MGameInstance.h"
#include "MInventorySlotWidget.h"
#include "MPickableItem.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/WrapBox.h"

void UMPickUpBarWidget::CreateSlots(TSet<UMInventoryComponent*>& InventoriesToRepresent)
{
	const auto pGameInstance = GetGameInstance<UMGameInstance>();
	if (!pGameInstance || !pItemSlotsWrapBox)
		return;

	if (!pGameInstance->ItemsDataAsset)
		return;
	const auto ItemsData = pGameInstance->ItemsDataAsset->ItemsData;

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
		int Index = 0;
		for (auto& [Item, OnChangedDelegate] : InventoryComponent->GetSlots())
		{
			const auto SlotWidget = CreateWidget<UMInventorySlotWidget>(this, ItemSlotWidgetBPClass);
			if (!SlotWidget)
				continue;

			SlotWidget->SetNumberInArray(Index++);
			SlotWidget->SetOwnerInventory(InventoryComponent);
			SlotWidget->SetOwnerInventoryWidget(nullptr);
			SlotWidget->SetStoredItem(Item);
			OnChangedDelegate.BindDynamic(SlotWidget, &UMInventorySlotWidget::OnChangedData);
			if (const auto InventoryOwner = Cast<AMPickableItem>(InventoryComponent->GetOwner());
				InventoryOwner && !OnChangedDelegate.IsBoundToObject(InventoryOwner))
			{
				OnChangedDelegate.BindDynamic(InventoryOwner, &AMPickableItem::OnItemChanged);
			}

			const auto IconWidget = Cast<UImage>(SlotWidget->GetWidgetFromName(TEXT("ItemIcon")));
			const auto QuantityTextWidget = Cast<URichTextBlock>(SlotWidget->GetWidgetFromName(TEXT("QuantityTextBlock")));
			if (IconWidget && QuantityTextWidget) // Icon and QuantityText widgets exist
				{
				if (Item.Quantity > 0 && Item.ID < ItemsData.Num())
				{
					// Item data is valid, don't draw quantity of a single item
					IconWidget->SetBrushFromTexture(ItemsData[Item.ID].IconTexture);
					IconWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					QuantityTextWidget->SetVisibility(ESlateVisibility::Hidden);

					if (Item.Quantity > 1) // Draw quantity text
					{
						QuantityTextWidget->SetText(FText::FromString(FString::FromInt(Item.Quantity)));
						QuantityTextWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					}
				}
				else // Slot is empty
					{
					IconWidget->SetVisibility(ESlateVisibility::Hidden);
					QuantityTextWidget->SetVisibility(ESlateVisibility::Hidden);
					}
				}
			if (pItemSlotsWrapBox)
			{
				pItemSlotsWrapBox->AddChild(SlotWidget);
			}
		}
	}
}
