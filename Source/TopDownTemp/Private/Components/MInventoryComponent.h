#pragma once

#include "CoreMinimal.h"
#include "MInventoryComponent.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnSlotChanged, int, NewItemID, int, NewQuantity);
DECLARE_MULTICAST_DELEGATE(FOnAnySlotChanged);

USTRUCT(BlueprintType)
struct FItem
{
	GENERATED_BODY()

	// BlueprintReadWrite access on purpose. We make it possible to play standalone with no internet.
	// Game will sync the time and remove all the items that was not possible to get (calculating the total cost)
	UPROPERTY(Category=FItem, EditAnywhere, BlueprintReadWrite)
	int ID = 0;

	UPROPERTY(Category=FItem, EditAnywhere, BlueprintReadWrite)
	int Quantity = 0;

	bool operator ==(const FItem& Other) const { return ID == Other.ID && Quantity == Other.Quantity; }
	bool operator !=(const FItem& Other) const { return !(*this == Other); }
};

USTRUCT(BlueprintType)
struct FSlot
{
	GENERATED_BODY()

	UPROPERTY()
	FItem Item;
	FOnSlotChanged OnSlotChangedDelegate;

	enum class ESlotFlags : uint8 {
		None = 0x00,
		Locked = 0x01,
		Secret = 0x02,
		PreviewOnly = 0x04
	};

	void SetFlag(ESlotFlags flag) {
		Flags = static_cast<ESlotFlags>(static_cast<uint8>(Flags) | static_cast<uint8>(flag));
	}

	void UnsetFlag(ESlotFlags flag) {
		Flags = static_cast<ESlotFlags>(static_cast<uint8>(Flags) & ~static_cast<uint8>(flag));
	}

	bool CheckFlag(ESlotFlags flag) const {
		return static_cast<uint8>(Flags) & static_cast<uint8>(flag);
	}

private:
	ESlotFlags Flags = ESlotFlags::None;
};


//TODO: Add additional checks for IsLocked in c++ functions. Now there are only some in the slot widget blueprint
/** A character's inventory component. Store items, support put-in, get-out and sort logic */
UCLASS(BlueprintType)
class TOPDOWNTEMP_API UMInventoryComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:

	void Initialize(int IN_SlotsNumber, const TArray<FItem>& StartingItems);

	TArray<FItem> GetItemCopies();

	static int GetTotallPrice(const TArray<FSlot>& Slots, const UObject* WorldContextObject);

	TArray<FItem> MaxPriceCombination(int M);
	//TArray<FItem> GetMaximumItemsForPrice(int Price);

	TArray<FSlot>& GetSlots() { return Slots; }

	static void SortSlots(TArray<FSlot>& IN_Slots, const UObject* WorldContextObject);

	static void SortItems(TArray<FItem>& IN_Items, const UObject* WorldContextObject);

	/** Combine any pair of items with same ID and not at max Quantity */
	static void StackItems(TArray<FItem>& IN_Items);

	/** Try to store the item in the inventory. If does not fit, drop it on the ground */
	UFUNCTION(BlueprintCallable)
	void StoreItem(const FItem& ItemToStore);

	UFUNCTION(BlueprintCallable)
	FItem StoreItemToSpecificSlot(int SlotNumberInArray, const FItem& ItemToStore);

	UFUNCTION(BlueprintCallable)
	FItem TakeItemFromSpecificSlot(int SlotNumberInArray, int Quantity);

	// The function is needed because we need to be sure there are enough items before pulling items out of different slots in TakeItem
	/** Check if there exist enough quantity of items with the given ID */
	bool DoesContainEnough(FItem ItemToCheck);

	/** Check if the given quantity of items with given ID fit in the inventory */
	bool IsEnoughSpace(FItem ItemToCheck, const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable)
	void RemoveItem(FItem ItemToRemove);

	UFUNCTION(BlueprintCallable)
	void SwapItems(UPARAM(ref)FItem& A, int SlotNumberInArray);

	void Empty();

	void SetFlagToAllSlots(FSlot::ESlotFlags Flag);

	FOnAnySlotChanged OnAnySlotChangedDelegate;

protected:

	TArray<FSlot> Slots;
};