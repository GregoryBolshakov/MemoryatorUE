#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MInventoryComponent.h"
#include "MInventoryWidget.generated.h"

class AMPickableItem;
class UWrapBox;

//TODO: Try the feature: take the full stack on double click;
//TODO: Scroll when the dragged item is near the top/bottom side of the scrollbox.
UCLASS()
class TOPDOWNTEMP_API UMInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable)
	void CreateSlots();

	UFUNCTION(BlueprintCallable)
	bool OnDraggedItemDropped(const FItem& Item);

protected:

	UPROPERTY(EditDefaultsOnly, Category=MInventoryWidgetSettings)
	TSubclassOf<UUserWidget> ItemSlotWidgetBPClass;

	UPROPERTY(BlueprintReadWrite, Category=MInventoryWidgetSettings)
	UWrapBox* pItemSlotsWrapBox;

	UPROPERTY(BlueprintReadWrite, Category=MInventoryWidgetSettings)
	bool IsDraggedItemOutside;

	UPROPERTY(EditDefaultsOnly, Category=MInventoryWidgetSettings)
	TSubclassOf<AMPickableItem> AMPickableItemToSpawnClass;
};
