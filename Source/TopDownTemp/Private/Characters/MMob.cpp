#include "MMob.h"

#include "Components/MInventoryComponent.h"
#include "Controllers/MHostileMobController.h"
#include "Framework/MGameInstance.h"

AMMob::AMMob(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AMMob::BeginPlay()
{
	Super::BeginPlay();

	if (const auto pGameInstance = GetGameInstance<UMGameInstance>())
	{
		if (const auto CharacterSpeciesDataAsset = pGameInstance->CharacterSpeciesDataAsset)
		{
			// Temporary solution. We will do this only when spawn a new character. Already existing will load the SpeciesName from save
			const auto AllPossibleSpecies = CharacterSpeciesDataAsset->GetAllNamesByClass(GetClass());
			if (!AllPossibleSpecies.IsEmpty())
			{
				SpeciesName = AllPossibleSpecies[FMath::RandRange(0, AllPossibleSpecies.Num()-1)];
			}
		}

		if (InventoryComponent->GetSlots().IsEmpty())
		{
			GenerateStartingInventory();
		}
	}
}

void AMMob::GenerateStartingInventory()
{
	const auto pGameInstance = GetGameInstance<UMGameInstance>();
	if (InventoryComponent)
	{
		if (const auto pCharacterSpeciesDataAsset = pGameInstance->CharacterSpeciesDataAsset)
		{
			if (const auto MyData = pCharacterSpeciesDataAsset->Data.Find(SpeciesName))
			{
				TArray<FItem> StartingInventory;
				for (const auto [ItemID, ItemData] : MyData->StartingItemsData)
				{
					if (FMath::RandRange(0.f, 1.f) <= ItemData.Probability)
					{
						StartingInventory.Add({ItemID, FMath::RandRange(ItemData.MinMaxStartQuantity.X, ItemData.MinMaxStartQuantity.Y)});
					}
				}
				InventoryComponent->Initialize(StartingInventory.Num(), StartingInventory);
			}
		}
		InventoryComponent->SetFlagToAllSlots(FSlot::ESlotFlags::Secret);
		InventoryComponent->SetFlagToAllSlots(FSlot::ESlotFlags::Locked);
	}
}
