#include "MShopItemWidget.h"

#include "Framework/MGameInstance.h"
#include "NakamaManager/Private/NakamaManager.h"
#include "NakamaManager/Private/ShopManagerClient.h"


void UMShopItemWidget::OnButtonClicked()
{
	if (const auto GameInstance = GetGameInstance<UMGameInstance>())
	{
		if (const auto NakamaManager = GameInstance->GetNakamaManager())
		{
			if (const auto ShopManager = NakamaManager->ShopManagerClient)
			{
				ShopManager->BuyBundle(BundleID);
			}
		}
	}
}
