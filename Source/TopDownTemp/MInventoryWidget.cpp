#include "MInventoryWidget.h"
#include "MCharacter.h"
#include "MGameInstance.h"
#include "MInventoryComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"

void UMInventoryWidget::CreateSlots()
{
	const auto pPlayerCharacter = Cast<AMCharacter>(GetOwningPlayerPawn());
	if (!IsValid(pPlayerCharacter) || !ItemSlotWidgetBPClass)
		return;

	const auto InventoryComponent = pPlayerCharacter->GetInventoryComponent();
	if (!InventoryComponent)
		return;

	const auto pGameInstance = GetGameInstance<UMGameInstance>();
	if (!pGameInstance)
		return;

	if (!pGameInstance->ItemsDataAsset)
		return;
	const auto ItemsData = pGameInstance->ItemsDataAsset->ItemsData;

	// Create widgets for player's inventory slots
	for (const auto& Item : InventoryComponent->GetSlots())
	{
		const auto SlotWidget = CreateWidget(this, ItemSlotWidgetBPClass);

		const auto IconWidget = Cast<UImage>(SlotWidget->GetWidgetFromName(TEXT("ItemIcon")));
		const auto QuantityTextWidget = Cast<UTextBlock>(SlotWidget->GetWidgetFromName(TEXT("QuantityTextBlock")));
		if (IconWidget && QuantityTextWidget) // Icon and QuantityText widgets exist
		{
			if (Item.Quantity > 0 && Item.ID < ItemsData.Num())
			{
				// Item data is valid, don't draw quantity of a single item
				IconWidget->SetBrushFromTexture(ItemsData[Item.ID].IconTexture);
				IconWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

				if (Item.Quantity > 1) // Draw quantity text
				{
					QuantityTextWidget->SetText(FText::FromString(FString::FromInt(Item.Quantity)));
					QuantityTextWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}

				SlotWidget->SetIsEnabled(true);
			}
			else // Slot is empty
			{
				IconWidget->SetVisibility(ESlateVisibility::Hidden);
				QuantityTextWidget->SetVisibility(ESlateVisibility::Hidden);
				SlotWidget->SetIsEnabled(false);
			}
		}
		if (pItemSlotsWrapBox)
		{
			pItemSlotsWrapBox->AddChild(SlotWidget);
		}
	}

}
