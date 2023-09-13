#include "MSaveManager.h"

#include "MBlockGenerator.h"
#include "MSaveTypes.h"
#include "MWorldGenerator.h"
#include "Characters/MCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "StationaryActors/MActor.h"
#include "StationaryActors/MPickableActor.h"
#include "StationaryActors/MGroundBlock.h"

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
	const auto SaveGameWorld = Cast<USaveGameWorld>(UGameplayStatics::CreateSaveGameObject(USaveGameWorld::StaticClass()));
	if (!SaveGameWorld || !WorldGenerator)
		return;

	for (const auto& [BlockIndex, pBlockOfActors] : GridOfActors.GetMap())
	{
		if (!SaveGameWorld->SavedGrid.Contains(BlockIndex))
		{
			SaveGameWorld->SavedGrid.Add(BlockIndex, {});
		}
		const auto SavedBlock = SaveGameWorld->SavedGrid.Find(BlockIndex);
		for (const auto& [Name, pActor] : pBlockOfActors->StaticActors)
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
		for (const auto& [Name, pActor] : pBlockOfActors->DynamicActors)
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

	// Access player's block to raise their block on top of the priority queue
	//TODO: store a pointer to the player in worldgenerator to guarantee its validity here
	if (const auto pPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		GridOfActors.Get(WorldGenerator->GetGroundBlockIndex(pPlayer->GetActorLocation()));
	}

	SaveGameWorld->GridOrder = GridOfActors.GetCacheOrder();

	UGameplayStatics::SaveGameToSlot(SaveGameWorld, USaveGameWorld::SlotName, 0);
}

bool UMSaveManager::AsyncLoadFromMemory(AMWorldGenerator* WorldGenerator)
{
	//TODO: Consider explicit destroying GridOfActors if loading in the middle of the game

	LoadedGameWorld = Cast<USaveGameWorld>(UGameplayStatics::LoadGameFromSlot(USaveGameWorld::SlotName, 0));

	if (!LoadedGameWorld || !WorldGenerator)
		return false;

	LoadPerTick(WorldGenerator);

	return true;
}

//TODO: Ensure priority of loaded blocks over generated in AMWorldGenerator::OnTickGenerateBlocks
void UMSaveManager::LoadPerTick(AMWorldGenerator* WorldGenerator)
{
	constexpr int BlocksPerFrame = 4;
	int Index = 0;
	for (auto It = LoadedGameWorld->GridOrder.CreateIterator(); It; ++It)
	{
		const auto Block = LoadedGameWorld->SavedGrid.Find(*It);
		check(Block);
		LoadBlock(*It, *Block, WorldGenerator);
		It.RemoveCurrent();

		if (++Index >= BlocksPerFrame)
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick([this, WorldGenerator]{ LoadPerTick(WorldGenerator); });
			break;
		}
	}
}

void UMSaveManager::LoadBlock(const FIntPoint& BlockIndex, const FBlockSaveData& BlockSD, AMWorldGenerator* WorldGenerator)
{
	const auto BlockOfActors = WorldGenerator->EmptyBlock(BlockIndex, false, true);
	BlockOfActors->Biome = BlockSD.Biome;

	for (const auto& MActorSD : BlockSD.SavedMActors)
	{
		LoadMActor(MActorSD, WorldGenerator);
	}

	// Process ground block biome
	for (const auto [Name, StaticActor] : BlockOfActors->StaticActors)
	{
		if (const auto GroundBlock = Cast<AMGroundBlock>(StaticActor))
		{
			BlockOfActors->pGroundBlock = GroundBlock;
			BlockOfActors->pGroundBlock->UpdateBiome(BlockSD.Biome);
			break;
		}
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
	return WorldGenerator->SpawnActor<AMActor>(ActorSD.FinalClass, ActorSD.Location, ActorSD.Rotation, Params, true);
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

	return WorldGenerator->SpawnActor<AMCharacter>(ActorSD.FinalClass, ActorSD.Location, ActorSD.Rotation, Params, true);
}
