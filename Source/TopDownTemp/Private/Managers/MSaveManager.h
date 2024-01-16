#pragma once

#include "CoreMinimal.h"
#include "MSaveTypes.h"
#include "MSaveManager.generated.h"

class AMActor;
class AMCharacter;
class AMPickableActor;
class AMWorldGenerator;
class USaveGameWorld;
class UBlockMetadata;
struct FLRUCache;
struct FBlockSaveData;
struct FActorWorldMetadata;
struct FActorSaveData;
struct FMActorSaveData;
struct FMCharacterSaveData;

DECLARE_LOG_CATEGORY_EXTERN(LogSaveManager, Log, All);

UCLASS()
class UMSaveManager : public UObject
{
	GENERATED_BODY()

public:

	void SetUpAutoSaves(TMap<FIntPoint, UBlockMetadata*>& GridOfActors, AMWorldGenerator* WorldGenerator);
	void SaveToMemory(TMap<FIntPoint, UBlockMetadata*>& GridOfActors, AMWorldGenerator* WorldGenerator);
	bool LoadFromMemory();
	bool TryLoadBlock(const FIntPoint& BlockIndex, AMWorldGenerator* WorldGenerator);
	const FBlockSaveData* GetBlockData(const FIntPoint& Index) const;
	void RemoveBlock(const FIntPoint& Index);
	TArray<FIntPoint> GetPlayerTraveledPath() const;
	bool IsLoaded() const { return LoadedGameWorld != nullptr; }

private:

	AMActor* LoadMActor(const FMActorSaveData& MActorSD, AMWorldGenerator* WorldGenerator);
	AMCharacter* LoadMCharacter(const FMCharacterSaveData& MCharacterSD, AMWorldGenerator* WorldGenerator);

	FTimerHandle AutoSavesTimer;

	//TODO: To support truly endless worlds we should split save into several files.
	//TODO: Each will be responsible for a large chunk/region of the world, e.g. 1000 by 1000 blocks
	//TODO: I.e. we're gonna store saved regions in TMap<FIntPoint, USaveGameWorld*> LoadedWorldRegions
	//TODO: When player enters a region, mark it as dirty
	//TODO: When Saving, iterate all dirty/marked regions, save them and reset their dirty/marked state

	//TODO: OR, just limit the world
	UPROPERTY()
	USaveGameWorld* LoadedGameWorld;

	// Maps for quick access by name

	/** Matches FUid with pointers to FMActorSaveData stored in all blocks in the LoadedGameWorld.\n\n NOT A UPROPERTY\n\n
	 * May contain dangling pointers as blocks' save data might be removed, always validate results. */
	TMap<FUid, FMActorSaveData*> LoadedMActorMap;

	/** Matches FUid with pointers to FMCharacterSaveData stored in all blocks in the LoadedGameWorld.\n NOT A UPROPERTY\n\n
	 * May contain dangling pointers as blocks' save data might be removed, always validate results. */
	TMap<FUid, FMCharacterSaveData*> LoadedMCharacterMap;
};

