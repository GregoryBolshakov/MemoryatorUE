#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "MShopItemWidget.generated.h"

UCLASS(Blueprintable, BlueprintType)
class TOPDOWNTEMP_API UMShopItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetBundleID(uint32 IN_BundleID) { BundleID = IN_BundleID; }

protected:

	UFUNCTION(BlueprintCallable)
	void OnButtonClicked();

	uint32 BundleID;
};
