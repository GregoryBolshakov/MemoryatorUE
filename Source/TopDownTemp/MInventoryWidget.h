#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MInventoryWidget.generated.h"

class AMPickableItem;
class UWrapBox;

//TODO: Scroll when the dragged item is near the top/bottom side of the scrollbox.
UCLASS()
class TOPDOWNTEMP_API UMInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable)
	static void CreateItemSlotWidgets(UUserWidget* pOwner, UMInventoryComponent* pInventoryComponent, UWrapBox* pItemSlotsWrapBox);

protected:

	UPROPERTY(BlueprintReadWrite, Category=MInventoryWidgetSettings)
	UWrapBox* pItemSlotsWrapBox;
};
