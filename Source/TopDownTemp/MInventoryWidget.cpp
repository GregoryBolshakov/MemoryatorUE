#include "MInventoryWidget.h"
#include "MCharacter.h"
#include "MDropManager.h"
#include "MGameInstance.h"
#include "MInventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/WrapBox.h"
#include "MInventoryComponent.h"

void UMInventoryWidget::NativeDestruct()
{
	Super::NativeDestruct();

	if (const auto pPlayerCharacter = Cast<AMCharacter>(GetOwningPlayerPawn()))
	{
		if (const auto InventoryComponent = pPlayerCharacter->GetInventoryComponent())
		{
			for (auto& ItemSlot : InventoryComponent->GetSlots())
			{
				ItemSlot.OnSlotChangedDelegate.Unbind();
			}
		}
	}
}

void UMInventoryWidget::CreateItemSlotWidgets(UUserWidget* pOwner, UMInventoryComponent* pInventoryComponent, UWrapBox* pItemSlotsWrapBox)
{
	if (!pInventoryComponent || !pItemSlotsWrapBox || !UMDropManager::gItemSlotWidgetBPClass)
	{
		check(false);
		return;
	}

	const auto pGameInstance = pOwner->GetGameInstance<UMGameInstance>();
	if (!pGameInstance)
		return;

	if (!pGameInstance->ItemsDataAsset)
		return;
	const auto ItemsData = pGameInstance->ItemsDataAsset->ItemsData;

	int Index = 0;
	// Create widgets for player's inventory slots
	for (auto& [Item, OnChangedDelegate, IsLocked, IsSecret] : pInventoryComponent->GetSlots())
	{
		const auto SlotWidget = CreateWidget<UMInventorySlotWidget>(pOwner, UMDropManager::gItemSlotWidgetBPClass);
		if (!SlotWidget)
			continue;

		SlotWidget->SetNumberInArray(Index++);
		SlotWidget->SetOwnerInventory(pInventoryComponent);
		SlotWidget->SetStoredItem(Item);
		SlotWidget->SetIsLocked(IsLocked);
		SlotWidget->SetIsSecret(IsSecret);
		OnChangedDelegate.BindDynamic(SlotWidget, &UMInventorySlotWidget::OnChangedData);

		const auto IconWidget = Cast<UImage>(SlotWidget->GetWidgetFromName(TEXT("ItemIcon")));
		const auto QuantityTextWidget = Cast<URichTextBlock>(SlotWidget->GetWidgetFromName(TEXT("QuantityTextBlock")));
		if (IconWidget && QuantityTextWidget) // Icon and QuantityText widgets exist
		{
			if (!IsSecret)
			{
				if (Item.Quantity > 0 && Item.ID < ItemsData.Num())
				{
					// Item data is valid, don't draw quantity of a single item
					IconWidget->SetBrushFromTexture(ItemsData[Item.ID].IconTexture);
					IconWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

					QuantityTextWidget->SetText(FText::FromString(FString::FromInt(Item.Quantity)));
					QuantityTextWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}
				else // Slot is empty
				{
					IconWidget->SetVisibility(ESlateVisibility::Hidden);
					QuantityTextWidget->SetVisibility(ESlateVisibility::Hidden);
				}
			}
			else
			{
				IconWidget->SetBrushFromTexture(pGameInstance->ItemsDataAsset->UnknownIconTexture);
				IconWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				QuantityTextWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		pItemSlotsWrapBox->AddChild(SlotWidget);
	}
}