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
			// Empty in case they've been there since last load
			SavedBlock.SavedMActors.Empty();
			SavedBlock.SavedMPickableActors.Empty();
			SavedBlock.SavedMCharacters.Empty();

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
							SavedBlock.SavedMPickableActors.Add(MPickableActorSD);
						}
					}
					else
					{
						SavedBlock.SavedMActors.Add(MActorSD);
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

bool UMSaveManager::TryLoadBlock(const FIntPoint& BlockIndex, AMWorldGenerator* WorldGenerator)
{
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

	for (const auto& MPickableActorDS : BlockSD->SavedMPickableActors)
	{
		LoadMPickableActor(MPickableActorDS, WorldGenerator);
	}

	for (const auto& MCharacterDS : BlockSD->SavedMCharacters)
	{
		LoadMCharacter(MCharacterDS, WorldGenerator);
	}

	RemoveBlock(BlockIndex); // The block is loaded, it's save is unnecessary
	return true;
}

void UMSaveManager::RemoveBlock(const FIntPoint& Index) const
{
	if (LoadedGameWorld)
	{
		LoadedGameWorld->SavedGrid.Remove(Index);
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
		// If it is the player, make it possessed by the player controller
		if (Params.Name == FName("Player"))
		{
			if (const auto pPlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
			{
				pPlayerController->Possess(SpawnedCharacter);
			}
			else check(false);
		}

		return SpawnedCharacter;
	}

	check(false);
	return nullptr;
}
