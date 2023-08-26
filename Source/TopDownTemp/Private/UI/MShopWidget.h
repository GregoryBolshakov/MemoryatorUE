#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NakamaManager/Private/ShopManagerClient.h"
#include "MShopWidget.generated.h"

class UWrapBox;

UCLASS()
class TOPDOWNTEMP_API UMShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;

protected:

	void CreateProductWidgets(const TMap<uint32, FBundle>& Bundles);

	UPROPERTY(BlueprintReadWrite, Category=MInventoryWidgetSettings)
	UWrapBox* pItemSlotsWrapBox;

	UPROPERTY(EditDefaultsOnly, Category=MInventoryWidgetSettings)
	TSubclassOf<UUserWidget> ShopItemWidgetPBClass;
};
