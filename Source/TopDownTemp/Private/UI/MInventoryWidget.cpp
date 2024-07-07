#include "MInventoryWidget.h"
#include "Characters/MCharacter.h"
#include "Managers/MDropManager.h"
#include "Framework/MGameInstance.h"
#include "MInventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/WrapBox.h"
#include "Components/MInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Managers/MDropControllerComponent.h"

void UMInventoryWidget::NativeDestruct()
{
	Super::NativeDestruct();

	// TODO: Think of some way of unbinding delegates from inventory slots but exclusively for corresponding SlotWidgets 
}

void UMInventoryWidget::CreateSlots(const UMInventoryComponent* InventoryComponent)
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

	CreateItemSlotWidgets(this, InventoryComponent, pItemSlotsWrapBox);
}

void UMInventoryWidget::CreateItemSlotWidgets(UUserWidget* pOwner, const UMInventoryComponent* pInventoryComponent,
                                                 UWrapBox* pItemSlotsWrapBox)
{
	if (!pInventoryComponent || !pItemSlotsWrapBox || !UMDropManager::gItemSlotWidgetBPClass)
	{
		check(false);
		return;
	}

	const auto pGameInstance = pOwner->GetGameInstance<UMGameInstance>();
	if (!pGameInstance || !pGameInstance->ItemsDataAsset)
	{
		return;
	}

	const auto ItemsData = pGameInstance->ItemsDataAsset->ItemsData;

	// Create widgets for player's inventory slots
	for (int Index = 0; Index < pInventoryComponent->GetSlotsConst().Num(); ++Index)
	{
		const auto& Slot = pInventoryComponent->GetSlotsConst()[Index];
		const auto SlotWidget = CreateWidget<UMInventorySlotWidget>(pOwner, UMDropManager::gItemSlotWidgetBPClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetNumberInArray(Index);
		SlotWidget->SetOwnerInventory(pInventoryComponent);
		SlotWidget->SetStoredItem(Slot.Item);
		SlotWidget->SetIsLocked(Slot.CheckFlag(FSlot::ESlotFlags::Locked));
		SlotWidget->SetIsSecret(Slot.CheckFlag(FSlot::ESlotFlags::Secret));
		SlotWidget->SetIsPreviewOnly(Slot.CheckFlag(FSlot::ESlotFlags::PreviewOnly));
		if (pInventoryComponent->GetOwner()->HasAuthority())
		{
			if (auto* InventoryComponentMutable = const_cast<UMInventoryComponent*>(pInventoryComponent))
			{
				auto& SlotMutable = InventoryComponentMutable->GetSlots()[Index];
				SlotMutable.OnSlotChangedDelegate.RemoveDynamic(SlotWidget, &UMInventorySlotWidget::OnChangedData);
				SlotMutable.OnSlotChangedDelegate.AddDynamic(SlotWidget, &UMInventorySlotWidget::OnChangedData);
			}
		}

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
