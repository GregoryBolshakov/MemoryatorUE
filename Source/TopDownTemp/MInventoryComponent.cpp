#include "MInventoryComponent.h"

#include "MDropManager.h"
#include "MGameInstance.h"
#include "MWorldGenerator.h"
#include "MWorldManager.h"

UMInventoryComponent::UMInventoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMInventoryComponent::Initialize(int IN_SlotsNumber, const TArray<FItem>& StartingItems)
{
	Slots.Empty();
	Slots.AddDefaulted(IN_SlotsNumber);
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

void UMInventoryComponent::StoreItem(const FItem& ItemToStore)
{
	FItem ItemLeft = ItemToStore;

	const auto pWorld = GetWorld();
	if (!IsValid(pWorld)) { check(false); return; }

	const auto pMGameInstance = GetWorld()->GetGameInstance<UMGameInstance>();
	if (!IsValid(pMGameInstance) || !pMGameInstance->ItemsDataAsset)
		return;

	const auto ItemsData = pMGameInstance->ItemsDataAsset->ItemsData;
	if (ItemToStore.ID >= ItemsData.Num())
	{
		check(false);
		return;
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

			Slot.OnSlotChangedDelegate.ExecuteIfBound(Slot.Item.ID, Slot.Item.Quantity);
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

				Slot.OnSlotChangedDelegate.ExecuteIfBound(Slot.Item.ID, Slot.Item.Quantity);
			}

			if (ItemLeft.Quantity == 0)
			{
				break;
			}
		}
	}

	if (ItemToStore.Quantity != ItemLeft.Quantity)
	{
		OnAnySlotChangedDelegate.Broadcast();
	}
	if (ItemLeft.Quantity == 0)
		return;

	// Item doesn't fit in the inventory, drop it on the ground
	if (const auto WorldManager = GetWorld()->GetSubsystem<UMWorldManager>())
	{
		if (const auto WorldGenerator = WorldManager->GetWorldGenerator())
		{
			if (const auto DropManager = WorldGenerator->GetDropManager())
			{
				DropManager->SpawnPickableItem(ItemLeft);
			}
		}
	}
}

FItem UMInventoryComponent::StoreItemToSpecificSlot(int SlotNumberInArray, const FItem& ItemToStore)
{
	// Validity checks---------------
	if (Slots.Num() <= SlotNumberInArray)
		return ItemToStore;

	if (Slots[SlotNumberInArray].Item.Quantity > 0 && Slots[SlotNumberInArray].Item.ID != ItemToStore.ID)
		return ItemToStore;

	const auto pMGameInstance = GetWorld()->GetGameInstance<UMGameInstance>();
    if (!IsValid(pMGameInstance) || !pMGameInstance->ItemsDataAsset)
    	return ItemToStore;
    
    const auto ItemsData = pMGameInstance->ItemsDataAsset->ItemsData;
    if (ItemToStore.ID >= ItemsData.Num())
    {
    	check(false);
    	return ItemToStore;
    }

	const auto MaxStack = ItemsData[Slots[SlotNumberInArray].Item.ID].MaxStack;
	const auto QuantityToStore = FMath::Min(ItemToStore.Quantity, MaxStack - Slots[SlotNumberInArray].Item.Quantity);
	check(QuantityToStore >= 0);

	if (QuantityToStore == 0)
		return ItemToStore;
	//-------------------------------

	Slots[SlotNumberInArray].Item.ID = ItemToStore.ID;
	Slots[SlotNumberInArray].Item.Quantity += QuantityToStore;
	Slots[SlotNumberInArray].OnSlotChangedDelegate.ExecuteIfBound(Slots[SlotNumberInArray].Item.ID, Slots[SlotNumberInArray].Item.Quantity);

	auto ItemToReturn = ItemToStore;
	ItemToReturn.Quantity -= QuantityToStore;

	OnAnySlotChangedDelegate.Broadcast();

	return ItemToReturn;
}

FItem UMInventoryComponent::TakeItemFromSpecificSlot(int SlotNumberInArray, int Quantity)
{
	//TODO: Should be replicated and do validation

	if (Slots.Num() <= SlotNumberInArray || Quantity == 0)
	{
		check(false);
		return {};
	}

	const int QuantityToTake = FMath::Min(Quantity, Slots[SlotNumberInArray].Item.Quantity);

	Slots[SlotNumberInArray].Item.Quantity -= QuantityToTake;

	Slots[SlotNumberInArray].OnSlotChangedDelegate.ExecuteIfBound(Slots[SlotNumberInArray].Item.ID, Slots[SlotNumberInArray].Item.Quantity);

	OnAnySlotChangedDelegate.Broadcast();

	check(QuantityToTake != 0);
	return {Slots[SlotNumberInArray].Item.ID, QuantityToTake};
}

void UMInventoryComponent::SwapItems(FItem& A, int SlotNumberInArray)
{
	if (SlotNumberInArray >= Slots.Num())
		return;

	Swap(A, Slots[SlotNumberInArray].Item);

	if (A.ID != Slots[SlotNumberInArray].Item.ID || A.Quantity != Slots[SlotNumberInArray].Item.Quantity) // At least any difference
	{
		OnAnySlotChangedDelegate.Broadcast();
	}

	Slots[SlotNumberInArray].OnSlotChangedDelegate.ExecuteIfBound(Slots[SlotNumberInArray].Item.ID, Slots[SlotNumberInArray].Item.Quantity);
}

void UMInventoryComponent::MakeAllItemsSecret()
{
	for (auto& Slot : Slots)
	{
		Slot.IsSecret = true;
	}
}

void UMInventoryComponent::LockAllItems()
{
	for (auto& Slot : Slots)
	{
		Slot.IsLocked = true;
	}
}