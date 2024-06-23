#include "MInventoryWidget.h"
#include "Characters/MCharacter.h"
#include "Managers/MDropManager.h"
#include "Framework/MGameInstance.h"
#include "MInventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/WrapBox.h"
#include "Components/MInventoryComponent.h"

void UMInventoryWidget::NativeDestruct()
{
	Super::NativeDestruct();

	if (const auto pPlayerCharacter = Cast<AMCharacter>(GetOwningPlayerPawn()))
	{
		if (const auto InventoryComponent = pPlayerCharacter->GetInventoryComponent())
		{
			// Unbind all bindings. It might be wrong, but looks right for now
			for (auto& ItemSlot : InventoryComponent->GetSlots())
			{
				ItemSlot.OnSlotChangedDelegate.Clear();
			}
		}
	}
}

void UMInventoryWidget::CreateItemSlotWidgets(UUserWidget* pOwner, UMInventoryComponent* pInventoryComponent,
                                              UWrapBox* pItemSlotsWrapBox)
{
	if (!pInventoryComponent || !pItemSlotsWrapBox || !UMDropManager::gItemSlotWidgetBPClass)
	{
		check(false);
		return;
	}

	const auto pGameInstance = pOwner->GetGameInstance<UMGameInstance>();
	if (!pGameInstance)
	{
		return;
	}

	if (!pGameInstance->ItemsDataAsset)
	{
		return;
	}
	const auto ItemsData = pGameInstance->ItemsDataAsset->ItemsData;

	int Index = 0;
	// Create widgets for player's inventory slots
	for (auto& Slot : pInventoryComponent->GetSlots())
	{
		const auto SlotWidget = CreateWidget<UMInventorySlotWidget>(pOwner, UMDropManager::gItemSlotWidgetBPClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetNumberInArray(Index++);
		SlotWidget->SetOwnerInventory(pInventoryComponent);
		SlotWidget->SetStoredItem(Slot.Item);
		SlotWidget->SetIsLocked(Slot.CheckFlag(FSlot::ESlotFlags::Locked));
		SlotWidget->SetIsSecret(Slot.CheckFlag(FSlot::ESlotFlags::Secret));
		SlotWidget->SetIsPreviewOnly(Slot.CheckFlag(FSlot::ESlotFlags::PreviewOnly));
		Slot.OnSlotChangedDelegate.RemoveDynamic(SlotWidget, &UMInventorySlotWidget::OnChangedData);
		Slot.OnSlotChangedDelegate.AddDynamic(SlotWidget, &UMInventorySlotWidget::OnChangedData);

		const auto IconWidget = Cast<UImage>(SlotWidget->GetWidgetFromName(TEXT("ItemIcon")));
		const auto QuantityTextWidget = Cast<URichTextBlock>(SlotWidget->GetWidgetFromName(TEXT("QuantityTextBlock")));
		if (IconWidget && QuantityTextWidget) // Icon and QuantityText widgets exist
		{
			if (!Slot.CheckFlag(FSlot::ESlotFlags::Secret))
			{
				if (Slot.Item.Quantity > 0 && Slot.Item.ID < ItemsData.Num() && Slot.Item.ID > 0)
				{
					// Item data is valid, don't draw quantity of a single item
					IconWidget->SetBrushFromTexture(ItemsData[Slot.Item.ID].IconTexture);
					IconWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

					QuantityTextWidget->SetText(FText::FromString(FString::FromInt(Slot.Item.Quantity)));
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
