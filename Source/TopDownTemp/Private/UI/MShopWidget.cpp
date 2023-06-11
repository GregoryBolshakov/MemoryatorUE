#include "MShopWidget.h"

#include "MShopItemWidget.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/WrapBox.h"
#include "Framework/MGameInstance.h"
#include "NakamaManager/Private/NakamaManager.h"

void UMShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (const auto GameInstance = GetGameInstance<UMGameInstance>())
	{
		if (const auto NakamaManager = GameInstance->GetNakamaManager())
		{
			CreateProductWidgets(NakamaManager->NakamaShopManager->GetBundles());
		}
	}
}

void UMShopWidget::CreateProductWidgets(const TMap<uint32, FBundle>& Bundles)
{
	const auto pGameInstance = GetGameInstance<UMGameInstance>();
	if (!pGameInstance)
		return;
	if (!pGameInstance->ItemsDataAsset)
		return;
	const auto& ItemsData = pGameInstance->ItemsDataAsset->ItemsData;

	for (auto& [ID, Bundle] : Bundles)
	{
		const auto SlotWidget = CreateWidget<UMShopItemWidget>(this, ShopItemWidgetPBClass);
		if (!SlotWidget)
			continue;

		SlotWidget->SetBundleID(ID);

		if (Bundle.items.Num() == 1) // Only one item for sale
		{
			const auto IconWidget = Cast<UImage>(SlotWidget->GetWidgetFromName(TEXT("ItemIcon")));
			const auto QuantityTextWidget = Cast<URichTextBlock>(SlotWidget->GetWidgetFromName(TEXT("QuantityTextBlock")));
			const auto PriceTextWidget = Cast<URichTextBlock>(SlotWidget->GetWidgetFromName(TEXT("PriceTextBlock")));
			if (IconWidget && QuantityTextWidget && PriceTextWidget && // widgets exist
				Bundle.items[0].qty > 0 && Bundle.items[0].itemid < static_cast<uint32>(ItemsData.Num()) && Bundle.items[0].itemid > 0) // valid item data
			{
				IconWidget->SetBrushFromTexture(ItemsData[Bundle.items[0].itemid].IconTexture);

				QuantityTextWidget->SetText(FText::FromString(FString::FromInt(Bundle.items[0].qty)));

				PriceTextWidget->SetText(FText::FromString("$" + FString::SanitizeFloat(Bundle.items[0].amount / 100.f))); // TODO: Add localization for the currency

				pItemSlotsWrapBox->AddChild(SlotWidget);
			}
		}

		//TODO: Process multiple items / non-inventory items (boosters/etc.)
	}
}
