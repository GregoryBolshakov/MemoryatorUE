#pragma once

#include "CoreMinimal.h"
#include "MInventoryComponent.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnSlotChanged, int, NewItemID, int, NewQuantity);

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
};

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="A character's inventory component. Store items, support put-in, get-out and sort logic"))
class TOPDOWNTEMP_API UMInventoryComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	void Initialize(int IN_SlotsNumber, const TArray<FItem>& StartingItems);

	TArray<FSlot>& GetSlots() { return Slots; }

	void SortItems(const TArray<struct FItemData>& ItemsData);

	UFUNCTION(BlueprintCallable)
	FItem StoreItem(const FItem& ItemToStore);

	UFUNCTION(BlueprintCallable)
	FItem StoreItemToSpecificSlot(int SlotNumberInArray, const FItem& ItemToStore);

	UFUNCTION(BlueprintCallable)
	FItem TakeItemFromSpecificSlot(int SlotNumberInArray, int Quantity);

	UFUNCTION(BlueprintCallable)
	void SwapItems(UPARAM(ref)FItem& A, int SlotNumberInArray);

protected:

	TArray<FSlot> Slots;
};