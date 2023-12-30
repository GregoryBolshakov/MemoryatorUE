#include "MSaveManager.h"

#include "MBlockGenerator.h"
#include "MSaveTypes.h"
#include "MWorldGenerator.h"
#include "Characters/MCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "StationaryActors/MActor.h"
#include "StationaryActors/MPickableActor.h"
#include "StationaryActors/MGroundBlock.h"

DEFINE_LOG_CATEGORY(LogSaveManager);

void UMSaveManager::SetUpAutoSaves(FLRUCache& GridOfActors, const AMWorldGenerator* WorldGenerator)
{
	const auto World = GetWorld();
	if (!IsValid(World))
		return;

	World->GetTimerManager().SetTimer(AutoSavesTimer, [this, &GridOfActors, WorldGenerator]
	{
		SaveToMemory(GridOfActors, WorldGenerator);
	}, 5.f, true);
}

void UMSaveManager::SaveToMemory(FLRUCache& GridOfActors, const AMWorldGenerator* WorldGenerator)
{
	const auto SaveGameWorld = LoadedGameWorld ? LoadedGameWorld : Cast<USaveGameWorld>(UGameplayStatics::CreateSaveGameObject(USaveGameWorld::StaticClass()));
	if (!SaveGameWorld || !WorldGenerator)
		return;

	// Access player's block to raise their block on top of the priority queue
	GridOfActors.Get(WorldGenerator->GetPlayerGroundBlockIndex());

	// Get the block indexes existing in the real world and prepend them to the saved cache order.
	// (Saved cache order is very likely to be much bigger than the real world's one)
	TSet<FIntPoint> CacheOrderSet(GridOfActors.GetCacheOrder());
	SaveGameWorld->GridOrder.RemoveAll([&](const FIntPoint& Point) {
		return CacheOrderSet.Contains(Point);
	});
	SaveGameWorld->GridOrder.Insert(GridOfActors.GetCacheOrder(), 0);

	// Iterate the LRU cache, saving blocks in descending priority order in case the process suddenly aborts
	for (const auto BlockIndex : GridOfActors.GetCacheOrder())
	{
		if (const auto BlockMetadata = GridOfActors.Get(BlockIndex))
		{
			FBlockSaveData* SavedBlock = SavedBlock = SaveGameWorld->SavedGrid.Find(BlockIndex);;
			if (!SavedBlock)
			{
				SavedBlock = &SaveGameWorld->SavedGrid.Add(BlockIndex, {});
			}

			SavedBlock->PCGVariables = BlockMetadata->pGroundBlock->PCGVariables;
			// Empty in case they've been there since last load
			SavedBlock->SavedMActors.Empty();
			SavedBlock->SavedMPickableActors.Empty();
			SavedBlock->SavedMCharacters.Empty();

			for (const auto& [Name, pActor] : BlockMetadata->StaticActors)
			{
				if (!IsValid(pActor) || Cast<AMGroundBlock>(pActor)) // Don't save ground blocks as we recreate them manually
					continue;

				// Start from the base and compose structs upwards
				FActorSaveData ActorSaveData{
					pActor->GetClass(),
					pActor->GetActorLocation(),
					pActor->GetActorRotation(),
					Name.ToString()
				};
				if (const auto pMActor = Cast<AMActor>(pActor))
				{
					FMActorSaveData MActorSD{
						ActorSaveData,
						pMActor->GetAppearanceID(),
						pMActor->GetIsRandomizedAppearance()
					};
					if (const auto pMPickableActor = Cast<AMPickableActor>(pActor))
					{
						if (const auto InventoryComponent = Cast<UMInventoryComponent>(pMPickableActor->GetComponentByClass(UMInventoryComponent::StaticClass())))
						{
							FMPickableActorSaveData MPickableActorSD{
								MActorSD,
								InventoryComponent->GetItemCopies(false)
							};
							SavedBlock->SavedMPickableActors.Add(MPickableActorSD);
						}
					}
					else
					{
						SavedBlock->SavedMActors.Add(MActorSD);
					}
				}
			}
			for (const auto& [Name, pActor] : BlockMetadata->DynamicActors)
			{
				if (!IsValid(pActor))
					continue;

				// Start from the base and compose structs upwards
				FActorSaveData ActorSaveData{
					pActor->GetClass(),
					pActor->GetActorLocation(),
					pActor->GetActorRotation(),
					Name.ToString()
				};
				if (const auto pMCharacter = Cast<AMCharacter>(pActor))
				{
					FMCharacterSaveData MCharacterSD{
						ActorSaveData,
						pMCharacter->GetSpeciesName(),
						pMCharacter->GetHealth()
					};
					SavedBlock->SavedMCharacters.Add(MCharacterSD);
				}
			}
		}
	}
	ensure(SaveGameWorld->SavedGrid.Num() == SaveGameWorld->GridOrder.Num());

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

	return true;
}

/*//TODO: Ensure priority of loaded blocks over generated in AMWorldGenerator::OnTickGenerateBlocks
void UMSaveManager::LoadPerTick(AMWorldGenerator* WorldGenerator)
{
	constexpr int BlocksPerFrame = 4;
	int Index;
	for (Index = BlockToLoadIndex; Index < FMath::Min(BlockToLoadIndex + BlocksPerFrame, LoadedGameWorld->GridOrder.Num()); ++Index)
	{
		const auto Block = LoadedGameWorld->SavedGrid.Find(LoadedGameWorld->GridOrder[Index]);
		check(Block);
		if (Block)
		{
			LoadBlock(LoadedGameWorld->GridOrder[Index], *Block, WorldGenerator);
		}
	}
	BlockToLoadIndex += BlocksPerFrame;

	if (Index < LoadedGameWorld->GridOrder.Num() - 1) // Haven't loaded all the blocks, live it for the next frame
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick([this, WorldGenerator]{ LoadPerTick(WorldGenerator); });
	} else
	{
		check(LoadedGameWorld);
		UE_LOG(LogSaveManager, Log, TEXT("Finished loading the world from the save"))
	}
}*/

void UMSaveManager::LoadBlock(const FIntPoint& BlockIndex, const FBlockSaveData& BlockSD, AMWorldGenerator* WorldGenerator)
{
	//TODO: For static terrain generation we're relying on PCG determinism for now, but be careful
	const auto BlockMetadata = WorldGenerator->EmptyBlock(BlockIndex, false, true);
	BlockMetadata->Biome = BlockSD.PCGVariables.Biome;
	WorldGenerator->GetBlockGenerator()->SpawnActorsSpecifically(BlockIndex, WorldGenerator, BlockSD.PCGVariables);

	for (const auto& MActorSD : BlockSD.SavedMActors)
	{
		LoadMActor(MActorSD, WorldGenerator);
	}

	for (const auto& MPickableActorDS : BlockSD.SavedMPickableActors)
	{
		LoadMPickableActor(MPickableActorDS, WorldGenerator);
	}

	for (const auto& MCharacterDS : BlockSD.SavedMCharacters)
	{
		LoadMCharacter(MCharacterDS, WorldGenerator);
	}
}

AMActor* UMSaveManager::LoadMActor(const FMActorSaveData& MActorSD, AMWorldGenerator* WorldGenerator)
{
	if (!WorldGenerator)
		return nullptr;

	const auto& ActorSD = MActorSD.ActorSaveData;

	FActorSpawnParameters Params;
	Params.Name = FName(ActorSD.NameString);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FOnSpawnActorStarted OnSpawnActorStarted;
	OnSpawnActorStarted.AddLambda([this, &MActorSD](AActor* Actor)
	{
		if (const auto MActor = Cast<AMActor>(Actor))
		{
			MActor->SetAppearanceID(MActorSD.AppearanceID);
			return;
		}
		check(false);
	});
	return WorldGenerator->SpawnActor<AMActor>(ActorSD.FinalClass, ActorSD.Location, ActorSD.Rotation, Params, false, OnSpawnActorStarted);
}

AMPickableActor* UMSaveManager::LoadMPickableActor(const FMPickableActorSaveData& MPickableActorSD,
	AMWorldGenerator* WorldGenerator)
{
	if (!WorldGenerator)
		return nullptr;

	if (const auto MPickableActor = Cast<AMPickableActor>(LoadMActor(MPickableActorSD.MActorSaveData, WorldGenerator)))
	{
		MPickableActor->InitialiseInventory(MPickableActorSD.InventoryContents);
		return MPickableActor;
	}
	return nullptr;
}

AMCharacter* UMSaveManager::LoadMCharacter(const FMCharacterSaveData& MCharacterSD, AMWorldGenerator* WorldGenerator)
{
	if (!WorldGenerator)
		return nullptr;

	const auto& ActorSD = MCharacterSD.ActorSaveData;
	FActorSpawnParameters Params;
	Params.Name = FName(ActorSD.NameString);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the character using saved data
	//TODO: Understand why passing a non-zero rotation causes issues with components bounds (box extent) (e.g. AttackPuddleComponent) 
	if (const auto SpawnedCharacter = WorldGenerator->SpawnActor<AMCharacter>(ActorSD.FinalClass, ActorSD.Location, /*ActorSD.Rotation*/ FRotator::ZeroRotator, Params, true))
	{
		// If it was the player, make it possessed by the player controller
		if (Params.Name == FName("Player"))
		{
			if (const auto pPlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
			{
				pPlayerController->Possess(SpawnedCharacter);
			}
		}

		return SpawnedCharacter;
	}

	check(false);
	return nullptr;
}
