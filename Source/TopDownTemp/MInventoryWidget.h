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

	virtual void PostInitProperties() override;

	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable)
	static void CreateSlots(UUserWidget* pOwner, UMInventoryComponent* pInventoryComponent, UWrapBox* pItemSlotsWrapBox);

	static TSubclassOf<UUserWidget> gItemSlotWidgetBPClass; // TODO: think about another place for this one

protected:

	UPROPERTY(EditDefaultsOnly, Category=MInventoryWidgetSettings)
	TSubclassOf<UUserWidget> ItemSlotWidgetBPClass;

	UPROPERTY(BlueprintReadWrite, Category=MInventoryWidgetSettings)
	UWrapBox* pItemSlotsWrapBox;

	UPROPERTY(BlueprintReadWrite, Category=MInventoryWidgetSettings)
	bool IsDraggedItemOutside;
};
