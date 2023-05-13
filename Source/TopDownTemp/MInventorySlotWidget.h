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

	void SetIsLocked(bool IN_IsLocked) { IsLocked = IN_IsLocked; }
	void SetIsSecret(bool IN_IsSecret) { IsSecret = IN_IsSecret; }

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

	// IsLocked and IsSecret don't take visual effect after setting, you need to reconstruct widget again to see the difference
	/** If the item cannot be taken or interacted with in any way */
	UPROPERTY(BlueprintReadOnly)
	bool IsLocked = false;

	/** If the item does not display its contents (ID, Quantity) */
	UPROPERTY(BlueprintReadOnly)
	bool IsSecret = false;
};

