#include "MShopItemWidget.h"

#include "Framework/MGameInstance.h"
#include "NakamaManager/Private/NakamaManager.h"
#include "NakamaManager/Private/NakamaShopManager.h"


void UMShopItemWidget::OnButtonClicked()
{
	if (const auto GameInstance = GetGameInstance<UMGameInstance>())
	{
		if (const auto NakamaManager = GameInstance->GetNakamaManager())
		{
			if (const auto ShopManager = NakamaManager->NakamaShopManager)
			{
				ShopManager->BuyBundle(BundleID);
			}
		}
	}
}
