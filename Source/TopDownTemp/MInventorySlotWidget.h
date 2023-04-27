#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MInventoryComponent.h"
#include "MInventorySlotWidget.generated.h"

class UMInventoryWidget;
class UMInventoryComponent;
class UMDropManager;

UCLASS(Blueprintable, BlueprintType)
class TOPDOWNTEMP_API UMInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;

	void SetNumberInArray(int IN_NumberInArray) { NumberInArray = IN_NumberInArray; }

	UFUNCTION(BlueprintCallable, Category=UMInventorySlotWidget)
	void SetOwnerInventory(UMInventoryComponent* IN_OwnerInventory) { OwnerInventory = IN_OwnerInventory; }

	void SetStoredItem(const FItem& IN_Item) { StoredItem = IN_Item; }

	UFUNCTION(BlueprintImplementableEvent)
	void OnChangedData(int NewItemID, int NewQuantity);

protected:

	/** Number of the slot (container) in the wrap box, it doesn't change even if we swap items! */
	UPROPERTY(BlueprintReadOnly, Category=UMInventorySlotWidget, meta=(AllowPrivateAccess=true))
	int NumberInArray;

	UPROPERTY(BlueprintReadOnly, Category=UMInventorySlotWidget, meta=(AllowPrivateAccess=true))
	UMInventoryComponent* OwnerInventory;

	/** The copy of the item. Is needed when we stack or swap with dragged item */
	UPROPERTY(BlueprintReadWrite)
	FItem StoredItem;

	/** A pointer for easier access of World Generator's Drop Manager */
	UPROPERTY(BlueprintReadOnly)
	UMDropManager* pDropManager;
};

