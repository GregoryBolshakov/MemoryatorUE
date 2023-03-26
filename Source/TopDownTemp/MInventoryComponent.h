#pragma once

#include "CoreMinimal.h"
#include "MInventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FItem
{
	GENERATED_BODY()
	int ID = 0;
	int Quantity = 0;
};

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="A character's inventory component. Store items, support put-in, get-out and sort logic"))
class TOPDOWNTEMP_API UMInventoryComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	void Initialize(int IN_SlotsNumber, const TArray<FItem>& StartingItems);

	UFUNCTION(BlueprintCallable)
	const TArray<FItem>& GetSlots() { return Slots; }

	void SortItems(const TArray<struct FItemData>& ItemsData);

	FItem StoreItem(const FItem& ItemToStore);

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=InventorySettings, meta=(AllowPrivateAccess=true))
	int SlotsNumber;

	TArray<FItem> Slots;
};