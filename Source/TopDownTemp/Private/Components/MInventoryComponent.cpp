#include "MInventoryComponent.h"

#include "Framework/MGameInstance.h"
#include "Managers/MDropManager.h"
#include "Managers/MWorldGenerator.h"
#include "Managers/MWorldManager.h"

UMInventoryComponent::UMInventoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMInventoryComponent::Initialize(int IN_SlotsNumber, const TArray<FItem>& StartingItems)
{
	for (auto Slot : Slots)
	{
		Slot.OnSlotChangedDelegate.Unbind();
	}
	Slots.Empty();
	Slots.AddDefaulted(IN_SlotsNumber);
	for (const auto& Item : StartingItems)
	{
		StoreItem(Item);
	}
}

TArray<FItem> UMInventoryComponent::GetItemCopies()
{
	TArray<FItem> Result;
	for (const auto& Slot : Slots)
	{
		if (Slot.Item.Quantity != 0)
		{
			Result.Add(Slot.Item);
		}
	}
	return Result;
}

UMItemsDataAsset* GetItemsDataAsset(const UObject* WorldContextObject)
{
	if (const auto pWorld = WorldContextObject->GetWorld())
	{
		if (const auto pGameInstance = pWorld->GetGameInstance<UMGameInstance>())
		{
			if (const auto pItemsDataAsset = pGameInstance->ItemsDataAsset)
			{
				return pItemsDataAsset;
			}
		}
	}
	check(false);
	return nullptr;
}

int UMInventoryComponent::GetTotallPrice(const TArray<FSlot>& Slots, const UObject* WorldContextObject)
{
	float Price = 0.f; // We do all the calculations in float and trunc the result
	if (const auto pItemsDataAsset = GetItemsDataAsset(WorldContextObject))
	{
		for (const auto ItemSlot : Slots)
		{
			if (ItemSlot.Item.Quantity <= 0)
				continue;
			if (pItemsDataAsset->ItemsData.Num() <= ItemSlot.Item.ID || ItemSlot.Item.ID <= 0)
			{
				check(false);
				continue;
			}
			const auto ItemData = pItemsDataAsset->ItemsData[ItemSlot.Item.ID];
			Price += static_cast<float>(ItemData.Price) * ItemSlot.Item.Quantity;
		}
	}

	return FMath::TruncToInt(Price);
}

int GetCost(int ID, const UObject* WorldContextObject)
{
	if (const auto pItemsDataAsset = GetItemsDataAsset(WorldContextObject))
	{
		if (ID > 0 &&  ID < pItemsDataAsset->ItemsData.Num())
		{
			return pItemsDataAsset->ItemsData[ID].Price;
		}
	}

	check(false);
	return 0;
}

TArray<FItem> UMInventoryComponent::MaxPriceCombination(int M)
{
	TArray<FItem> NonZeroItems;
	for (const auto Slot : Slots)
	{
		if (Slot.Item.Quantity > 0)
		{
			NonZeroItems.Add({Slot.Item});
		}
	}

	int n = NonZeroItems.Num();
	TArray<TArray<int>> dp;
	dp.SetNum(n + 1);

	for (int i = 0; i < n + 1; i++)
	{
		dp[i].SetNum(M + 1);
	}

	TArray<TArray<TArray<FItem>>> picks;
	picks.SetNum(n + 1);

	for (int i = 0; i < n + 1; i++)
	{
		picks[i].SetNum(M + 1);
	}

	for (int i = 0; i <= n; i++)
	{
		for (int j = 0; j <= M; j++)
		{
			if (i == 0 || j == 0)
			{
				dp[i][j] = 0;
			}
			else
			{
				int cost = GetCost(NonZeroItems[i - 1].ID, GetWorld());
				if (cost <= j)
				{
					int maxValWithoutCurr = dp[i - 1][j];
					int maxValWithCurr = 0;
					int maxQtyWithCurr = 0;

					for (int k = 1; k <= NonZeroItems[i - 1].Quantity && k * cost <= j; k++)
					{
						int valWithCurr = k * cost + dp[i - 1][j - k * cost];

						if (valWithCurr > maxValWithCurr)
						{
							maxValWithCurr = valWithCurr;
							maxQtyWithCurr = k;
						}
					}

					if (maxValWithoutCurr > maxValWithCurr)
					{
						dp[i][j] = maxValWithoutCurr;

						if (i > 0 && j > 0)
						{
							picks[i][j] = picks[i - 1][j];
						}
					}
					else
					{
						dp[i][j] = maxValWithCurr;

						if (i > 0 && j > 0)
						{
							picks[i][j] = picks[i - 1][j - maxQtyWithCurr * cost];
							FItem item = NonZeroItems[i - 1];
							item.Quantity = maxQtyWithCurr;
							picks[i][j].Add(item);
						}
					}
				}
				else
				{
					dp[i][j] = dp[i - 1][j];

					if (i > 0 && j > 0)
					{
						picks[i][j] = picks[i - 1][j];
					}
				}
			}
		}
	}

	return picks[n][M];
}

void UMInventoryComponent::SortSlots(TArray<FSlot>& IN_Slots, const UObject* WorldContextObject)
{
	//aaaaaa!z
	if (const auto ItemsDataAsset = GetItemsDataAsset(WorldContextObject))
	{
		const auto ItemsData = ItemsDataAsset->ItemsData;
		IN_Slots.Sort([&ItemsData](const FSlot& A, const FSlot& B)
		{
			if (ItemsData.Num() <= A.Item.ID || ItemsData.Num() <= B.Item.ID || A.Item.ID <= 0 || B.Item.ID <= 0)
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
				return A.Item.Quantity > B.Item.Quantity;
			}
			return A.Item.ID < B.Item.ID;
		});
	}
}

void UMInventoryComponent::SortItems(TArray<FItem>& IN_Items, const UObject* WorldContextObject)
{
	if (const auto ItemsDataAsset = GetItemsDataAsset(WorldContextObject))
	{
		const auto ItemsData = ItemsDataAsset->ItemsData;
		IN_Items.Sort([&ItemsData](const FItem& A, const FItem& B)
		{
			if (ItemsData.Num() <= A.ID || ItemsData.Num() <= B.ID || A.ID <= 0 || B.ID <= 0)
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
				return A.Quantity > B.Quantity;
			}
			return A.ID < B.ID;
		});
	}
}

void UMInventoryComponent::StackItems(TArray<FItem>& IN_Items)
{
	// Create a map to hold the combined quantities for each unique ID.
	TMap<int, int> IDToQuantity;

	// Iterate over the input array.
	for (const auto& Item : IN_Items)
	{
		// If the ID already exists in the map, add the current quantity.
		// If it doesn't exist, this will initialize it with the current quantity.
		IDToQuantity.FindOrAdd(Item.ID) += Item.Quantity;
	}

	// Create an array to hold the combined items.
	TArray<FItem> StackedItems;

	// Iterate over the map and add each ID/quantity pair as an item in the new array.
	for (const auto& pair : IDToQuantity)
	{
		FItem StackedItem;
		StackedItem.ID = pair.Key;
		StackedItem.Quantity = pair.Value;
		StackedItems.Add(StackedItem);
	}

	IN_Items = StackedItems;
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
	if (ItemToStore.ID >= ItemsData.Num() || ItemToStore.ID <= 0) // ID valid check
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
	if (Slots.Num() <= SlotNumberInArray || ItemToStore.Quantity <= 0 || ItemToStore.ID <= 0)
		return ItemToStore;

	if (Slots[SlotNumberInArray].Item.Quantity > 0 && Slots[SlotNumberInArray].Item.ID != ItemToStore.ID)
		return ItemToStore;

	const auto pMGameInstance = GetWorld()->GetGameInstance<UMGameInstance>();
    if (!IsValid(pMGameInstance) || !pMGameInstance->ItemsDataAsset)
    	return ItemToStore;
    
    const auto ItemsData = pMGameInstance->ItemsDataAsset->ItemsData;
    if (ItemToStore.ID >= ItemsData.Num() || ItemToStore.ID <= 0) // ID valid check
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

bool UMInventoryComponent::DoesContainEnough(FItem ItemToCheck)
{
	if (ItemToCheck.Quantity == 0) {check(false); return true;}
	for (auto Slot : Slots)
	{
		if (Slot.Item.ID == ItemToCheck.ID)
		{
			const int QuantityToTake = FMath::Min(Slot.Item.Quantity, ItemToCheck.Quantity);
			ItemToCheck.Quantity -= QuantityToTake;

			if (ItemToCheck.Quantity == 0)
			{
				return true;
			}
		}
	}
	return false;
}

bool UMInventoryComponent::IsEnoughSpace(FItem ItemToCheck, const UObject* WorldContextObject)
{
	if (ItemToCheck.Quantity == 0) { check(false); return true; }

	if (const auto ItemsDataAsset = GetItemsDataAsset(WorldContextObject))
	{
		const auto ItemsData = ItemsDataAsset->ItemsData;
		if (ItemsData.Num() <= ItemToCheck.ID || ItemToCheck.ID <= 0) { check(false); return false; }
		const auto MaxStack = ItemsData[ItemToCheck.ID].MaxStack;
		for (int i = 0; i < Slots.Num(); ++i)
		{
			if (Slots[i].Item.Quantity == 0) // The slot is empty
			{
				ItemToCheck.Quantity -= MaxStack;
			}
			else
			if (Slots[i].Item.ID == ItemToCheck.ID)
			{
				ItemToCheck.Quantity -= FMath::Max(MaxStack - Slots[i].Item.Quantity, 0);
			}

			if (ItemToCheck.Quantity <= 0)
				return true;
		}
		return false;
	}
	check(false);
	return false;
}

void UMInventoryComponent::RemoveItem(FItem ItemToRemove)
{
	if (ItemToRemove.Quantity == 0 || !DoesContainEnough(ItemToRemove))
	{
		check(false);
		return;
	}

	for (int i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].Item.ID == ItemToRemove.ID)
		{
			const int QuantityToTake = FMath::Min(Slots[i].Item.Quantity, ItemToRemove.Quantity);
			Slots[i].Item.Quantity -= QuantityToTake;
			ItemToRemove.Quantity -= QuantityToTake;

			Slots[i].OnSlotChangedDelegate.ExecuteIfBound(Slots[i].Item.ID, Slots[i].Item.Quantity);
			OnAnySlotChangedDelegate.Broadcast();

			if (ItemToRemove.Quantity == 0)
			{
				return;
			}
		}
	}
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

void UMInventoryComponent::Empty()
{
	// In current implementation we keep the inventory capacity but just remove all the items
	for (int i = 0; i < Slots.Num(); ++i)
	{
		Slots[i].Item = {0, 0};
		Slots[i].OnSlotChangedDelegate.ExecuteIfBound(0, 0);
		OnAnySlotChangedDelegate.Broadcast();
	}
}

void UMInventoryComponent::SetFlagToAllSlots(FSlot::ESlotFlags Flag)
{
	for (auto& Slot : Slots)
	{
		Slot.SetFlag(Flag);
	}
}