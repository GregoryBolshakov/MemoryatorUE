#pragma once

#include "CoreMinimal.h"
#include "MInventoryComponent.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnSlotChanged, int, NewItemID, int, NewQuantity);
DECLARE_MULTICAST_DELEGATE(FOnAnySlotChanged);

USTRUCT(BlueprintType)
struct FItem
{
	GENERATED_BODY()

	UPROPERTY(Category=FItem, EditAnywhere, BlueprintReadOnly)
	int ID = 0;

	UPROPERTY(Category=FItem, EditAnywhere, BlueprintReadOnly)
	int Quantity = 0;
};

USTRUCT(BlueprintType)
struct FSlot
{
	GENERATED_BODY()
	FItem Item;
	FOnSlotChanged OnSlotChangedDelegate;
	bool IsLocked = false; //TODO: think about a flag. Might be secret but show quantity, whatever
	bool IsSecret = false;
};


//TODO: Add additional checks for IsLocked in c++ functions. Now there are only some in the slot widget blueprint
/** A character's inventory component. Store items, support put-in, get-out and sort logic */
UCLASS(BlueprintType)
class TOPDOWNTEMP_API UMInventoryComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:

	void Initialize(int IN_SlotsNumber, const TArray<FItem>& StartingItems);

	TArray<FSlot>& GetSlots() { return Slots; }

	void SortItems(const TArray<struct FItemData>& ItemsData);

	/** Try to store the item in the inventory. If does not fit, drop it on the ground */
	UFUNCTION(BlueprintCallable)
	void StoreItem(const FItem& ItemToStore);

	UFUNCTION(BlueprintCallable)
	FItem StoreItemToSpecificSlot(int SlotNumberInArray, const FItem& ItemToStore);

	UFUNCTION(BlueprintCallable)
	FItem TakeItemFromSpecificSlot(int SlotNumberInArray, int Quantity);

	UFUNCTION(BlueprintCallable)
	void SwapItems(UPARAM(ref)FItem& A, int SlotNumberInArray);

	/** From external observers, e.g. the peeping player */
	void MakeAllItemsSecret();

	/** Prohibit interaction with items, e.g. player takes something out.
	 *  Called for every mob by default, any unlocked items may be stolen */
	void LockAllItems();

	FOnAnySlotChanged OnAnySlotChangedDelegate;

protected:

	TArray<FSlot> Slots;
};