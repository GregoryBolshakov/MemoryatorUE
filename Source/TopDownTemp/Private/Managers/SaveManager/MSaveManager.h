#pragma once

#include "CoreMinimal.h"
#include "MWorldSaveTypes.h"
#include "MUid.h"
#include "MSaveManager.generated.h"

class UActorWorldMetadata;
class AMActor;
class AMCharacter;
class AMPickableActor;
class AMWorldGenerator;
class USaveGameWorld;
class UBlockMetadata;
struct FLRUCache;
struct FBlockSaveData;
struct FActorSaveData;
struct FMActorSaveData;
struct FMCharacterSaveData;

DECLARE_LOG_CATEGORY_EXTERN(LogSaveManager, Log, All);

UCLASS()
class UMSaveManager : public UObject
{
	GENERATED_BODY()

public: // TODO: Stop using WorldGenerator as a parameter. It's accessible from the AMGameMode

	FMUid GenerateUid();
	FMUid FindMUidByUniqueID(FName UniqueID) const;
	void AddMUidByUniqueID(FName UniqueID, const FMUid& Uid) const;

	/** Read file with save and fill reversed mappings for loading purposes. */
	void LoadFromMemory();

	void SetUpAutoSaves(AMWorldGenerator* WorldGenerator);
	void SaveToMemory(AMWorldGenerator* WorldGenerator);
	bool TryLoadBlock(const FIntPoint& BlockIndex, AMWorldGenerator* WorldGenerator);
	const FBlockSaveData* GetBlockData(const FIntPoint& Index) const;

	/** Get MCharacter's ground block index without explicitly loading and spawning it.\n
	 * Especially useful for processing connecting players for initial surroundings */
	const FIntPoint GetMCharacterBlock(const FMUid& Uid) const;

	void RemoveBlock(const FIntPoint& Index);

	bool IsLoaded() const;

	AMActor* LoadMActorAndClearSD(const FMUid& Uid);
	AMCharacter* LoadMCharacterAndClearSD(const FMUid& Uid);

	static TMap<FString, FComponentSaveData> GetSaveDataForComponents(const AActor* Actor);

private:
	AMActor* LoadMActor_Internal(const FMActorSaveData& MActorSD);
	AMCharacter* LoadMCharacter_Internal(const FMCharacterSaveData& MCharacterSD);

	void ClearMActorSD(const FMUid& Uid, const FIntPoint& BlockIndex);
	void ClearMCharacterSD(const FMUid& Uid, const FIntPoint& BlockIndex);

	void LoadDataForComponents(AActor* Actor, const TMap<FString, FComponentSaveData>& ComponentsSD);

	FTimerHandle AutoSavesTimer;

	//TODO: To support truly endless worlds we should split save into several files.
	//TODO: Each will be responsible for each region (or larger).
	//TODO: I.e. we're gonna store saved regions in TMap<FIntPoint, USaveGameWorld*> LoadedWorldRegions
	//TODO: When player enters a region, mark it as dirty
	//TODO: When Saving, iterate all dirty/marked regions, save them and reset their dirty/marked state

	//TODO: OR, just limit the world
	UPROPERTY()
	USaveGameWorld* LoadedGameWorld;

private: // Relations between saved actors

	//TODO: Wrap into separate manager like it's done with MMetadataManager-----------------------------
	/** Matches FUid with pointers to FMActorSaveData stored in all blocks in the LoadedGameWorld.*/
	TMap<FMUid, FMActorSaveData*> LoadedMActorMap;

	/** Matches FUid with pointers to FMCharacterSaveData stored in all blocks in the LoadedGameWorld.*/
	TMap<FMUid, FMCharacterSaveData*> LoadedMCharacterMap;
	//TODO: --------------------------------------------------------------------------------------------

	UPROPERTY()
	TMap<FMUid, AActor*> AlreadySpawnedSavedActors;
	// Заметки:
	// Если акер уже заспавнился, он спровоцировал спавн всех зависимых актеров (виладжер спавнится -> спавнится дом)
	// При вызове SaveToMemory() всё дерево зависимостей уже существует в AMWorldGenerator::ActorsMetadata

	TAtomic<int32> NumberUniqueIndex = MAX_int32; //TODO: Should also reset between different launches in the editor

	/** Mapping between project's FMUid and Unreal player network ID.\n
	* Just reversed version of USaveGameWorld's UniqueIDToMUid. Need to skip loading players on blocks.\n
	* Filled at startup, no need to save, as it's relatively small and duplicates all data in already saved field. */
	UPROPERTY()
	TMap<FMUid, FName> MUidToUniqueID;
};

