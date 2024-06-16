#include "MSaveManager.h"

#include "Managers/MBlockGenerator.h"
#include "Managers/RoadManager/MRoadManager.h"
#include "Managers/MWorldGenerator.h"
#include "Managers/MMetadataManager.h"
#include "Characters/MCharacter.h"
#include "Characters/MMemoryator.h"
#include "Components/MIsActiveCheckerComponent.h"
#include "Components/MStatsModelComponent.h"
#include "Controllers/MMobControllerBase.h"
#include "Framework/MGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "StationaryActors/MActor.h"
#include "StationaryActors/MPickableActor.h"
#include "StationaryActors/MGroundBlock.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"

DEFINE_LOG_CATEGORY(LogSaveManager);

FMUid UMSaveManager::GenerateUid()
{
	return {LoadedGameWorld->LaunchId, --NumberUniqueIndex};
}

void UMSaveManager::SetUpAutoSaves(AMWorldGenerator* WorldGenerator)
{
	const auto* World = GetWorld();
	if (!IsValid(World))
		return;

	World->GetTimerManager().SetTimer(AutoSavesTimer, [this, WorldGenerator]
	{
		SaveToMemory(WorldGenerator);
	}, 5.f, true);
}

void UMSaveManager::SaveToMemory(AMWorldGenerator* WorldGenerator)
{
	/*if (!IsValid(LoadedGameWorld))
	{
		LoadedGameWorld = Cast<USaveGameWorld>(UGameplayStatics::CreateSaveGameObject(USaveGameWorld::StaticClass()));
	}
	if (!WorldGenerator)
		return;

	WorldGenerator->CheckDynamicActorsBlocks();

	// Iterate the world grid saving blocks
	//TODO: Mark visited(or modified) blocks as dirty and then iterate only marked
	auto* GridOfActors = AMGameMode::GetMetadataManager(this)->GetGrid();
	for (const auto& [BlockIndex, BlockMetadata] : *GridOfActors)
	{
		// We don't consider blocks without actors to be generated, even if they are marked with some biome
		if (BlockMetadata && (!BlockMetadata->StaticActors.IsEmpty() || !BlockMetadata->DynamicActors.IsEmpty()) || BlockMetadata->pGroundBlock)
		{
			auto& SavedBlock = LoadedGameWorld->SavedGrid.FindOrAdd(BlockIndex);

			if (BlockMetadata->pGroundBlock)
			{
				SavedBlock.PCGVariables = BlockMetadata->pGroundBlock->PCGVariables;
			}
			SavedBlock.ConstantActorsCount = BlockMetadata->ConstantActorsCount;
			// Empty in case they've been there since last load
			SavedBlock.SavedMActors.Empty();
			SavedBlock.SavedMCharacters.Empty();

			for (const auto& [Name, pActor] : BlockMetadata->StaticActors)
			{
				const auto* pActorMetadata = AMGameMode::GetMetadataManager(this)->Find(Name);
				if (!IsValid(pActor) || !pActorMetadata)
				{
					continue;
				}
				// Don't save ground blocks as we recreate them manually
				if (Cast<AMGroundBlock>(pActor))
				{
					continue;
				}

				// Start from the base and compose structs upwards
				FActorSaveData ActorSaveData = {
					pActor->GetClass(),
					pActor->GetActorLocation(),
					pActor->GetActorRotation(),
					pActorMetadata->Uid,
					GetSaveDataForComponents(pActor)
				};
				if (const auto* pMActor = Cast<AMActor>(pActor))
				{
					FMActorSaveData MActorSD{
						ActorSaveData,
						pMActor->GetAppearanceID(),
						pMActor->GetIsRandomizedAppearance()
					};
					// Save inventory if MActor has it
					if (auto* InventoryComponent = Cast<UMInventoryComponent>(pMActor->GetComponentByClass(UMInventoryComponent::StaticClass())))
					{
						MActorSD.InventoryContents = InventoryComponent->GetItemCopies(false);
					}
					SavedBlock.SavedMActors.Add({ActorSaveData.Uid, MActorSD});
				}
			}
			for (const auto& [Name, pActor] : BlockMetadata->DynamicActors)
			{
				auto* pActorMetadata = AMGameMode::GetMetadataManager(this)->Find(Name);
				if (!IsValid(pActor) || !pActorMetadata)
				{
					continue;
				}

				// Start from the base and compose structs upwards
				FActorSaveData ActorSaveData = {
					pActor->GetClass(),
					pActor->GetActorLocation(),
					pActor->GetActorRotation(),
					pActorMetadata->Uid,
					GetSaveDataForComponents(pActor)
				};
				if (const auto* pMCharacter = Cast<AMCharacter>(pActor))
				{
					FMCharacterSaveData MCharacterSD{
						ActorSaveData,
						pMCharacter->GetSpeciesName(),
						pMCharacter->GetStatsModelComponent()->GetHealth(),
					};
					// Save inventory if MCharacter has it
					if (auto* InventoryComponent = Cast<UMInventoryComponent>(pMCharacter->GetComponentByClass(UMInventoryComponent::StaticClass())))
					{
						MCharacterSD.InventoryContents = InventoryComponent->GetItemCopies(false);
					}
					// Save house if has it
					if (const auto* House = pMCharacter->GetHouse())
					{
						if (const auto* HouseMetadata = AMGameMode::GetMetadataManager(this)->Find(FName(House->GetName())))
						{
							MCharacterSD.HouseUid = HouseMetadata->Uid;
						}
					}
					SavedBlock.SavedMCharacters.Add({ActorSaveData.Uid, MCharacterSD});
				}
			}
		}
	}

	AMGameMode::GetRoadManager(this)->SaveToMemory();

	check(LoadedGameWorld);
	if (LoadedGameWorld)
	{
		UGameplayStatics::SaveGameToSlot(LoadedGameWorld, USaveGameWorld::SlotName, 0);
	}*/
}

void UMSaveManager::LoadFromMemory()
{
	LoadedGameWorld = Cast<USaveGameWorld>(UGameplayStatics::LoadGameFromSlot(USaveGameWorld::SlotName, 0));
	if (!LoadedGameWorld)
	{
		LoadedGameWorld = Cast<USaveGameWorld>(UGameplayStatics::CreateSaveGameObject(USaveGameWorld::StaticClass()));
		return;
	}

	LoadedGameWorld->LaunchId--;

	// Fill mappings between Uid and Saved Data
	for (auto& [Index, BlockSD] : LoadedGameWorld->SavedGrid)
	{
		for (auto& [Uid, MActorSD] : BlockSD.SavedMActors)
		{
			LoadedMActorMap.Add(MActorSD.ActorSaveData.Uid, &MActorSD);
		}
		for (auto& [Uid, MCharacterSD] : BlockSD.SavedMCharacters)
		{
			LoadedMCharacterMap.Add(MCharacterSD.ActorSaveData.Uid, &MCharacterSD);
		}
	}

	// Fill reverse mapping for UniqueID and Uid
	for (const auto& [UniqueID, Uid] : LoadedGameWorld->UniqueIDToMUid)
	{
		MUidToUniqueID.Add(Uid, UniqueID);
	}
}

bool UMSaveManager::TryLoadBlock(const FIntPoint& BlockIndex, AMWorldGenerator* WorldGenerator)
{
	if (!LoadedGameWorld)
		return false;
	const auto BlockSD = LoadedGameWorld->SavedGrid.Find(BlockIndex);
	if (!BlockSD)
		return false; // Either wasn't saved at all or is already loaded

	// THE PREVIOUS CONTENTS OF THE BLOCK ARE NOT DELETED AND NOT GUARANTEED TO BE EMPTY. BECAUSE WE NEED TO BE ABLE TO LOAD ACTORS INDIVIDUALLY.
	// WE RELY ON ALWAYS CHECKING FOR THE PRESENCE OF A SAVE BEFORE GENERATING A BLOCK.
	const auto BlockMetadata = AMGameMode::GetMetadataManager(this)->FindOrAddBlock(BlockIndex);

	BlockMetadata->ConstantActorsCount = BlockSD->ConstantActorsCount;
	BlockMetadata->Biome = BlockSD->PCGVariables.Biome;
	//TODO: For static terrain generation we're relying on PCG determinism, be careful
	WorldGenerator->GetBlockGenerator()->SpawnActorsSpecifically(BlockIndex, WorldGenerator, BlockSD->PCGVariables);

	// Load MActors and clear save data
	while (!BlockSD->SavedMActors.IsEmpty())
	{
		const auto Num = BlockSD->SavedMActors.Num();
		LoadMActorAndClearSD(BlockSD->SavedMActors.CreateIterator().Key(), WorldGenerator);
		if (Num == BlockSD->SavedMActors.Num())
		{
			check(false);
			return false;
		}
	}

	// Load MCharacters and clear save data
	while (!BlockSD->SavedMCharacters.IsEmpty())
	{
		const auto Num = BlockSD->SavedMCharacters.Num();
		const auto* Character = LoadMCharacterAndClearSD(BlockSD->SavedMCharacters.CreateIterator().Key(), WorldGenerator);
		if (Num == BlockSD->SavedMCharacters.Num())
		{
			check(false); // Some character didn't clear is save data
			return false;
		}

		const auto Metadata = AMGameMode::GetMetadataManager(this)->Find(FName(Character->GetName()));
		// By current design, inactive players don't appear in the world with the rest of the block, but only when the player is connected. We are not like Rust
		if (const auto UniqueID = MUidToUniqueID.Find(Metadata->Uid))
		{
			if (const auto ActiveChecker = Character->GetComponentByClass<UMIsActiveCheckerComponent>())
			{
				ActiveChecker->SetAlwaysDisabled(true);
				ActiveChecker->DisableOwner();
			}
		}
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

const FIntPoint UMSaveManager::GetMCharacterBlock(const FMUid& Uid) const
{
	if (LoadedGameWorld)
	{
		if (const auto* AlreadySpawnedActorMetadata = AMGameMode::GetMetadataManager(this)->Find(Uid))
		{
			return AlreadySpawnedActorMetadata->GroundBlockIndex;
		}
		const auto* pMCharacterSD = LoadedMCharacterMap.Find(Uid);
		if (!pMCharacterSD)
		{
			check(false); // Actor's saved data was removed, but the object was never spawned
			return {};
		}
		return AMGameMode::GetWorldGenerator(this)->GetGroundBlockIndex((*pMCharacterSD)->ActorSaveData.Location);
	}
	check(false); // Shouldn't call it until the world savefile is loaded
	return {};
}

void UMSaveManager::RemoveBlock(const FIntPoint& Index)
{
	if (LoadedGameWorld)
	{
		if (const auto* BlockSD = LoadedGameWorld->SavedGrid.Find(Index))
		{
			// Clear Uid mappings
			for (auto& [Uid, MActorSD] : BlockSD->SavedMActors)
			{
				LoadedMActorMap.Remove(Uid);
			}
			for (auto& [Uid, MCharacterSD] : BlockSD->SavedMCharacters)
			{
				LoadedMActorMap.Remove(Uid);
			}
			// Remove block saved data
			LoadedGameWorld->SavedGrid.Remove(Index);
		}
	}
}

FMUid UMSaveManager::FindMUidByUniqueID(FName UniqueID) const
{
	if (const auto pUid = LoadedGameWorld->UniqueIDToMUid.Find(UniqueID))
	{
		return *pUid;
	}
	return {};
}

void UMSaveManager::AddMUidByUniqueID(FName UniqueID, const FMUid& Uid) const
{
	check(!LoadedGameWorld->UniqueIDToMUid.Contains(UniqueID));
	LoadedGameWorld->UniqueIDToMUid.Add(UniqueID, Uid);
}

bool UMSaveManager::IsLoaded() const
{
	if (!IsValid(LoadedGameWorld))
		return false;
	return !LoadedGameWorld->SavedGrid.IsEmpty();
}


AMActor* UMSaveManager::LoadMActor_Internal(const FMActorSaveData& MActorSD, AMWorldGenerator* WorldGenerator)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FOnSpawnActorStarted OnSpawnActorStarted;
	OnSpawnActorStarted.AddLambda([this, &MActorSD](AActor* Actor)
	{
		if (const auto MActor = Cast<AMActor>(Actor))
		{
			MActor->SetAppearanceID(MActorSD.AppearanceID);
			MActor->InitialiseInventory(MActorSD.InventoryContents);
		}
		else check(false);
	});

	const auto& ActorSD = MActorSD.ActorSaveData;
	if (const auto MActor = WorldGenerator->SpawnActor<AMActor>(ActorSD.FinalClass, ActorSD.Location, ActorSD.Rotation, Params, false, OnSpawnActorStarted, ActorSD.Uid))
	{
		if (!ActorSD.Components.IsEmpty())
		{
			LoadDataForComponents(MActor, ActorSD.Components);
		}
		return MActor;
	}
	check(false);
	return nullptr;
}

AMCharacter* UMSaveManager::LoadMCharacter_Internal(const FMCharacterSaveData& MCharacterSD, AMWorldGenerator* WorldGenerator)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FOnSpawnActorStarted OnSpawnActorStarted;
	OnSpawnActorStarted.AddLambda([this, &MCharacterSD](AActor* Actor)
	{
		if (const auto MCharacter = Cast<AMCharacter>(Actor))
		{
			MCharacter->BeginLoadFromSD(MCharacterSD);
		}
		else check(false);
	});

	const auto& ActorSD = MCharacterSD.ActorSaveData;
	// TODO: Fix skewed AttackPuddleComponent, PerimeterOutlineComponent, etc. when loading ActorSD.Rotation
	if (const auto MCharacter = WorldGenerator->SpawnActor<AMCharacter>(ActorSD.FinalClass, ActorSD.Location, /*ActorSD.Rotation*/ FRotator::ZeroRotator, Params, true, OnSpawnActorStarted, ActorSD.Uid))
	{
		// TODO: MCharacter->EndLoadFromSD(); if have use cases for EndLoadFromSD
		if (!ActorSD.Components.IsEmpty())
		{
			LoadDataForComponents(MCharacter, ActorSD.Components);
		}
		return MCharacter;
	}
	check(false);
	return nullptr;
}

void UMSaveManager::ClearMActorSD(const FMUid& Uid, const FIntPoint& BlockIndex)
{
	if (auto* BlockSD = LoadedGameWorld->SavedGrid.Find(BlockIndex))
	{
		BlockSD->SavedMActors.Remove(Uid);
	}
	LoadedMActorMap.Remove(Uid);
}

void UMSaveManager::ClearMCharacterSD(const FMUid& Uid, const FIntPoint& BlockIndex)
{
	if (auto* BlockSD = LoadedGameWorld->SavedGrid.Find(BlockIndex))
	{
		BlockSD->SavedMCharacters.Remove(Uid);
	}
	LoadedMCharacterMap.Remove(Uid);
}

TMap<FString, FComponentSaveData> UMSaveManager::GetSaveDataForComponents(AActor* Actor)
{
	TMap<FString, FComponentSaveData> ComponentsSD;
	TArray<USceneComponent*> Components;
	Actor->GetComponents<USceneComponent>(Components, false);

	for (const auto* Component : Components)
	{
		if (Component->ComponentHasTag("Saved"))
		{
			FComponentSaveData ComponentSD { Component->GetRelativeLocation(), Component->GetRelativeRotation() };
			ComponentsSD.Add(Component->GetName(), ComponentSD);
		}
	}

	return ComponentsSD;
}

void UMSaveManager::LoadDataForComponents(AActor* Actor, const TMap<FString, FComponentSaveData>& ComponentsSD)
{
	TArray<USceneComponent*> Components;
	Actor->GetComponents<USceneComponent>(Components, false);

	for (auto* Component : Components)
	{
		if (Component->ComponentHasTag("Saved"))
		{
			if (const auto ComponentSD = ComponentsSD.Find(Component->GetName()))
			{
				Component->SetRelativeLocation(ComponentSD->RelativeLocation);
				Component->SetRelativeRotation(ComponentSD->RelativeRotation);
			}
			else check(false);
		}
	}
}

AMActor* UMSaveManager::LoadMActorAndClearSD(const FMUid& Uid, AMWorldGenerator* WorldGenerator)
{
	check(IsUidValid(Uid));
	check(WorldGenerator);
	// Before loading an actor check if it was already loaded
	if (const auto* AlreadySpawnedActorMetadata = AMGameMode::GetMetadataManager(this)->Find(Uid))
	{
		const auto AlreadySpawnedMActor = Cast<AMActor>(AlreadySpawnedActorMetadata->Actor);
		check(AlreadySpawnedMActor);
		ClearMActorSD(Uid, WorldGenerator->GetGroundBlockIndex(AlreadySpawnedMActor->GetActorLocation()));
		return AlreadySpawnedMActor;
	}
	const auto* pMActorSD = LoadedMActorMap.Find(Uid);
	if (!pMActorSD)
	{
		check(false); // Actor's saved data was removed, but the object was never spawned
		return nullptr;
	}
	const auto MActorSD = *pMActorSD;

	const auto LoadedMActor = LoadMActor_Internal(*MActorSD, WorldGenerator);

	// Save data is deleted immediately after the actor is loaded.
	const auto BlockIndex = WorldGenerator->GetGroundBlockIndex(MActorSD->ActorSaveData.Location);
	ClearMActorSD(Uid, BlockIndex);

	return LoadedMActor;
}

AMCharacter* UMSaveManager::LoadMCharacterAndClearSD(const FMUid& Uid, AMWorldGenerator* WorldGenerator)
{
	check(IsUidValid(Uid));
	check(WorldGenerator);
	// Before loading an actor check if it was already loaded
	if (const auto* AlreadySpawnedActorMetadata = AMGameMode::GetMetadataManager(this)->Find(Uid))
	{
		const auto AlreadySpawnedMCharacter = Cast<AMCharacter>(AlreadySpawnedActorMetadata->Actor);
		check(AlreadySpawnedMCharacter);
		ClearMCharacterSD(Uid, WorldGenerator->GetGroundBlockIndex(AlreadySpawnedMCharacter->GetActorLocation()));
		return AlreadySpawnedMCharacter;
	}
	const auto* pMCharacterSD = LoadedMCharacterMap.Find(Uid);
	if (!pMCharacterSD)
	{
		check(false); // Actor's saved data was removed, but the object was never spawned
		return nullptr;
	}
	const auto MCharacterSD = *pMCharacterSD;

	const auto LoadedMCharacter = LoadMCharacter_Internal(*MCharacterSD, WorldGenerator);

	// Save data is deleted immediately after the actor is loaded.
	const auto BlockIndex = WorldGenerator->GetGroundBlockIndex(MCharacterSD->ActorSaveData.Location);
	ClearMCharacterSD(Uid, BlockIndex);

	return LoadedMCharacter;
}
