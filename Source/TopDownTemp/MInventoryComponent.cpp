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
	Slots.Sort([&ItemsData](const FSlot& A, const FSlot& B)
	{
		if (ItemsData.Num() <= A.Item.ID || ItemsData.Num() <= B.Item.ID)
		{
			check(false);
			return true;
		}
		if (A.Item.ID == B.Item.ID)
		{
			if (A.Item.Quantity == B.Item.Quantity)
			{
				return ItemsData[A.Item.ID].MaxStack > ItemsData[B.Item.ID].MaxStack;
			}
			else
			{
				return A.Item.Quantity > B.Item.Quantity;
			}
		}
		else
		{
			return A.Item.ID < B.Item.ID;
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

	for (auto& Slot : Slots)
	{
		if (Slot.Item.ID >= ItemsData.Num())
		{
			check(false);
			continue;
		}
		// Search for not empty and not full slots with same ID to stack
		if (Slot.Item.ID == ItemToStore.ID && Slot.Item.Quantity < ItemsData[ItemToStore.ID].MaxStack && Slot.Item.Quantity != 0)
		{
			const int QuantityToAdd = FMath::Min(ItemLeft.Quantity, ItemsData[ItemToStore.ID].MaxStack - Slot.Item.Quantity);
			Slot.Item.Quantity += QuantityToAdd;
			ItemLeft.Quantity -= QuantityToAdd;
		}

		if (ItemLeft.Quantity == 0)
		{
			break;
		}
	}

	if (ItemLeft.Quantity > 0)
	{
		for (auto& Slot : Slots)
		{
			// Search for empty slots
			if (Slot.Item.Quantity == 0)
			{
				const int QuantityToAdd = FMath::Min(ItemLeft.Quantity, ItemsData[ItemToStore.ID].MaxStack);
				Slot.Item.ID = ItemToStore.ID;
				Slot.Item.Quantity += QuantityToAdd;
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

FItem UMInventoryComponent::TakeItem(int SlotNumberInArray, int Quantity)
{
	//TODO: Should be replicated and do validation

	if (Slots.Num() <= SlotNumberInArray)
		return {};

	const int QuantityToTake = FMath::Min(Quantity, Slots[SlotNumberInArray].Item.Quantity);

	Slots[SlotNumberInArray].Item.Quantity -= QuantityToTake;

	Slots[SlotNumberInArray].OnSlotChangedDelegate.ExecuteIfBound(Slots[SlotNumberInArray].Item.ID, Slots[SlotNumberInArray].Item.Quantity);

	return {Slots[SlotNumberInArray].Item.ID, QuantityToTake};
}
