#include "MSaveManager.h"

#include "MBlockGenerator.h"
#include "MWorldGenerator.h"
#include "Characters/MCharacter.h"
#include "Characters/MMemoryator.h"
#include "Controllers/MMobControllerBase.h"
#include "Kismet/GameplayStatics.h"
#include "StationaryActors/MActor.h"
#include "StationaryActors/MPickableActor.h"
#include "StationaryActors/MGroundBlock.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"

DEFINE_LOG_CATEGORY(LogSaveManager);

void UMSaveManager::SetUpAutoSaves(TMap<FIntPoint, UBlockMetadata*>& GridOfActors, AMWorldGenerator* WorldGenerator)
{
	const auto World = GetWorld();
	if (!IsValid(World))
		return;

	World->GetTimerManager().SetTimer(AutoSavesTimer, [this, &GridOfActors, WorldGenerator]
	{
		SaveToMemory(GridOfActors, WorldGenerator);
	}, 5.f, true);
}

static TAtomic<int32> NumberUniqueIndex(MAX_int32 - 1000); //TODO: Maybe reset between different launches in the editor
void UMSaveManager::SaveToMemory(TMap<FIntPoint, UBlockMetadata*>& GridOfActors, AMWorldGenerator* WorldGenerator)
{
	const auto SaveGameWorld = LoadedGameWorld ? LoadedGameWorld : Cast<USaveGameWorld>(UGameplayStatics::CreateSaveGameObject(USaveGameWorld::StaticClass()));
	if (!SaveGameWorld || !WorldGenerator)
		return;

	WorldGenerator->CheckDynamicActorsBlocks();

	SaveGameWorld->PlayerTraveledPath = { WorldGenerator->GetPlayerGroundBlockIndex() }; // TODO: Save the whole path

	// Iterate the world grid saving blocks
	//TODO: Mark visited(or modified) blocks as dirty and then iterate only marked
	for (const auto& [BlockIndex, BlockMetadata] : GridOfActors)
	{
		if (BlockMetadata && BlockMetadata->pGroundBlock) // We don't consider blocks without actors to be generated, even if they are marked with some biome
		{
			auto& SavedBlock = SaveGameWorld->SavedGrid.FindOrAdd(BlockIndex);

			SavedBlock.PCGVariables = BlockMetadata->pGroundBlock->PCGVariables;
			SavedBlock.WasConstant = BlockMetadata->ConstantActorsCount > 0;
			// Empty in case they've been there since last load
			SavedBlock.SavedMActors.Empty();
			SavedBlock.SavedMCharacters.Empty();

			for (const auto& [Name, pActor] : BlockMetadata->StaticActors)
			{
				if (!IsValid(pActor) || Cast<AMGroundBlock>(pActor)) // Don't save ground blocks as we recreate them manually
					continue;

				// Start from the base and compose structs upwards
				FActorSaveData ActorSaveData = {
					pActor->GetClass(),
					pActor->GetActorLocation(),
					pActor->GetActorRotation(),
					{SaveGameWorld->LaunchId, --NumberUniqueIndex}
				};
				if (const auto pMActor = Cast<AMActor>(pActor))
				{
					FMActorSaveData MActorSD{
						ActorSaveData,
						pMActor->GetAppearanceID(),
						pMActor->GetIsRandomizedAppearance()
					};
					// Save inventory if MActor has it
					if (const auto InventoryComponent = Cast<UMInventoryComponent>(pMActor->GetComponentByClass(UMInventoryComponent::StaticClass())))
					{
						MActorSD.InventoryContents = InventoryComponent->GetItemCopies(false);
					}

					SavedBlock.SavedMActors.Add(MActorSD);
				}
			}
			for (const auto& [Name, pActor] : BlockMetadata->DynamicActors)
			{
				if (!IsValid(pActor))
					continue;

				// Start from the base and compose structs upwards
				FActorSaveData ActorSaveData = {
					pActor->GetClass(),
					pActor->GetActorLocation(),
					pActor->GetActorRotation(),
					{SaveGameWorld->LaunchId, --NumberUniqueIndex}
				};
				if (const auto pMCharacter = Cast<AMCharacter>(pActor))
				{
					FMCharacterSaveData MCharacterSD{
						ActorSaveData,
						pMCharacter->GetSpeciesName(),
						pMCharacter->GetHealth(),
					};
					// Save inventory if MCharacter has it
					if (const auto InventoryComponent = Cast<UMInventoryComponent>(pMCharacter->GetComponentByClass(UMInventoryComponent::StaticClass())))
					{
						MCharacterSD.InventoryContents = InventoryComponent->GetItemCopies(false);
					}
					// Save house if has it //TODO: Consider every AMCharacter having House, not only mobs. Also not sure if it should reside in the controller
					/*if (const auto House = pMCharacter->GetHouse())
					{
						if (const auto* HouseUid = NameToUidMap.Find(FName(House->GetName())))
						{
							MCharacterSD.HouseUid = *HouseUid;
						}
					}*/

					SavedBlock.SavedMCharacters.Add(MCharacterSD);
				}
			}
		}
	}

	check(SaveGameWorld);
	if (SaveGameWorld)
	{
		UGameplayStatics::SaveGameToSlot(SaveGameWorld, USaveGameWorld::SlotName, 0);
	}
}

bool UMSaveManager::LoadFromMemory()
{
	LoadedGameWorld = Cast<USaveGameWorld>(UGameplayStatics::LoadGameFromSlot(USaveGameWorld::SlotName, 0));
	if (!LoadedGameWorld)
		return false;

	LoadedGameWorld->LaunchId--;

	for (const auto& [Index, BlockSD] : LoadedGameWorld->SavedGrid)
	{
		for (auto& MActorSD : BlockSD.SavedMActors)
		{
			LoadedMActorMap.Add({MActorSD.ActorSaveData.SavedUid, const_cast<FMActorSaveData*>(&MActorSD)});
		}
		for (const auto& MCharcterSD : BlockSD.SavedMCharacters)
		{
			LoadedMCharacterMap.Add({MCharcterSD.ActorSaveData.SavedUid, const_cast<FMCharacterSaveData*>(&MCharcterSD)});
		}
	}

	return true;
}

bool UMSaveManager::TryLoadBlock(const FIntPoint& BlockIndex, AMWorldGenerator* WorldGenerator)
{
	//TODO: MUST TODO! Support dependencies between actors: e.g. villager and his home. Come up with architecture when accumulate more examples
	if (!LoadedGameWorld)
		return false;
	const auto BlockMetadata = WorldGenerator->EmptyBlock(BlockIndex, true, true);
	const auto BlockSD = LoadedGameWorld->SavedGrid.Find(BlockIndex);
	if (!BlockSD)
		return false;
	BlockMetadata->Biome = BlockSD->PCGVariables.Biome;
	//TODO: For static terrain generation we're relying on PCG determinism, be careful
	WorldGenerator->GetBlockGenerator()->SpawnActorsSpecifically(BlockIndex, WorldGenerator, BlockSD->PCGVariables);

	for (const auto& MActorSD : BlockSD->SavedMActors)
	{
		LoadMActor(MActorSD, WorldGenerator);
	}

	for (const auto& MCharacterDS : BlockSD->SavedMCharacters)
	{
		LoadMCharacter(MCharacterDS, WorldGenerator);
	}

	RemoveBlock(BlockIndex); // The block is loaded, it's save is unnecessary
	return true;
}

const FBlockSaveData* UMSaveManager::GetBlockData(const FIntPoint& Index) const
{
	if (LoadedGameWorld)
	{
		return LoadedGameWorld->SavedGrid.Find(Index);
	}
	return nullptr;
}

void UMSaveManager::RemoveBlock(const FIntPoint& Index)
{
	if (LoadedGameWorld)
	{
		if (const auto* BlockSD = LoadedGameWorld->SavedGrid.Find(Index))
		{
			// Clear Uid mappings
			for (auto& MActorSD : BlockSD->SavedMActors)
			{
				LoadedMActorMap.Remove(MActorSD.ActorSaveData.SavedUid);
			}
			for (auto& MCharacterSD : BlockSD->SavedMCharacters)
			{
				LoadedMActorMap.Remove(MCharacterSD.ActorSaveData.SavedUid);
			}
			// Remove block saved data
			LoadedGameWorld->SavedGrid.Remove(Index);
		}
	}
}

TArray<FIntPoint> UMSaveManager::GetPlayerTraveledPath() const
{
	if (LoadedGameWorld)
	{
		return LoadedGameWorld->PlayerTraveledPath;
	}
	return {};
}

AMActor* UMSaveManager::LoadMActor(const FMActorSaveData& MActorSD, AMWorldGenerator* WorldGenerator)
{
	if (!WorldGenerator)
		return nullptr;

	const auto& ActorSD = MActorSD.ActorSaveData;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FOnSpawnActorStarted OnSpawnActorStarted;
	OnSpawnActorStarted.AddLambda([this, &MActorSD](AActor* Actor)
	{
		if (const auto MActor = Cast<AMActor>(Actor))
		{
			MActor->SetAppearanceID(MActorSD.AppearanceID);
			MActor->InitialiseInventory(MActorSD.InventoryContents);
			return;
		}
		check(false);
	});
	const auto MActor = WorldGenerator->SpawnActor<AMActor>(ActorSD.FinalClass, ActorSD.Location, ActorSD.Rotation, Params, false, OnSpawnActorStarted);

	return MActor;
}

AMCharacter* UMSaveManager::LoadMCharacter(const FMCharacterSaveData& MCharacterSD, AMWorldGenerator* WorldGenerator)
{
	if (!WorldGenerator)
		return nullptr;

	const auto& ActorSD = MCharacterSD.ActorSaveData;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FOnSpawnActorStarted OnSpawnActorStarted;
	if (!MCharacterSD.InventoryContents.IsEmpty())
	{
		OnSpawnActorStarted.AddLambda([MCharacterSD](AActor* Actor)
		{
			if (const auto MCharacter = Cast<AMCharacter>(Actor))
			{
				MCharacter->BeginLoadFromSD(MCharacterSD);
			}
		});
	}

	// Spawn the character using saved data
	if (const auto SpawnedCharacter = WorldGenerator->SpawnActor<AMCharacter>(ActorSD.FinalClass, ActorSD.Location, /*ActorSD.Rotation*/ FRotator::ZeroRotator, Params, true, OnSpawnActorStarted))
	{
		SpawnedCharacter->EndLoadFromSD();
		return SpawnedCharacter;
	}

	check(false);
	return nullptr;
}
