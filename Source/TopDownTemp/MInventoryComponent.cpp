#include "MInventoryComponent.h"

#include "MGameInstance.h"

UMInventoryComponent::UMInventoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SlotsNumber(40)
{
}

void UMInventoryComponent::Initialize(int IN_SlotsNumber, const TArray<FItem>& StartingItems)
{
	Slots.SetNum(SlotsNumber);
	for (const auto& Item : StartingItems)
	{
		StoreItem(Item);
	}
}

void UMInventoryComponent::SortItems(const TArray<FItemData>& ItemsData)
{
	Slots.Sort([&ItemsData](const FItem& A, const FItem& B)
	{
		if (ItemsData.Num() <= A.ID || ItemsData.Num() <= B.ID)
		{
			check(false);
			return true;
		}
		if (A.ID == B.ID)
		{
			if (A.Quantity == B.Quantity)
			{
				return ItemsData[A.ID].MaxStack > ItemsData[B.ID].MaxStack;
			}
			else
			{
				return A.Quantity > B.Quantity;
			}
		}
		else
		{
			return A.ID < B.ID;
		}
	});
}

FItem UMInventoryComponent::StoreItem(const FItem& ItemToStore)
{
	FItem ItemLeft = ItemToStore;

	const auto pMGameInstance = GetWorld()->GetGameInstance<UMGameInstance>();
	if (!IsValid(pMGameInstance) || !pMGameInstance->ItemsDataAsset)
		return ItemLeft;

	const auto ItemsData = pMGameInstance->ItemsDataAsset->ItemsData;
	if (ItemToStore.ID >= ItemsData.Num())
	{
		check(false);
		return ItemLeft;
	}

	// We consider any slot as empty if its Quantity is 0, no matter what ID it has

	for (auto& Item : Slots)
	{
		if (Item.ID >= ItemsData.Num())
		{
			check(false);
			continue;
		}
		// Search for not empty and not full slots with same ID to stack
		if (Item.ID == ItemToStore.ID && Item.Quantity < ItemsData[ItemToStore.ID].MaxStack && Item.Quantity != 0)
		{
			const int QuantityToAdd = FMath::Min(ItemLeft.Quantity, ItemsData[ItemToStore.ID].MaxStack - Item.Quantity);
			Item.Quantity += QuantityToAdd;
			ItemLeft.Quantity -= QuantityToAdd;
		}

		if (ItemLeft.Quantity == 0)
		{
			break;
		}
	}

	if (ItemLeft.Quantity > 0)
	{
		for (FItem& Item : Slots)
		{
			// Search for empty slots
			if (Item.Quantity == 0)
			{
				const int QuantityToAdd = FMath::Min(ItemLeft.Quantity, ItemsData[ItemToStore.ID].MaxStack);
				Item.ID = ItemToStore.ID;
				Item.Quantity += QuantityToAdd;
				ItemLeft.Quantity -= QuantityToAdd;
			}

			if (ItemLeft.Quantity == 0)
			{
				break;
			}
		}
	}

	return ItemLeft;
}