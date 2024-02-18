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

inline bool IsUidValid(const FMUid& Uid) { return Uid.ObjectId > MIN_int32; }

UCLASS()
class UMSaveManager : public UObject
{
	GENERATED_BODY()

public:

	FMUid GenerateUid();

	void LoadFromMemory();
	void SetUpAutoSaves(AMWorldGenerator* WorldGenerator);
	void SaveToMemory(AMWorldGenerator* WorldGenerator);
	bool TryLoadBlock(const FIntPoint& BlockIndex, AMWorldGenerator* WorldGenerator);
	const FBlockSaveData* GetBlockData(const FIntPoint& Index) const;
	const FMActorSaveData* GetMActorData(const FMUid& Uid);
	const FMCharacterSaveData* GetMCharacterData(const FMUid& Uid);
	void RemoveBlock(const FIntPoint& Index);

	/** Returns an array of visited blocks by player left-right <=> earlier-later. Might be empty. */
	TArray<FIntPoint> GetPlayerTraveledPath() const;

	bool IsLoaded() const;

	AMActor* LoadMActorAndClearSD(const FMUid& Uid, AMWorldGenerator* WorldGenerator);
	AMCharacter* LoadMCharacterAndClearSD(const FMUid& Uid, AMWorldGenerator* WorldGenerator);

private:
	AMActor* LoadMActor_Internal(const FMActorSaveData& MActorSD, AMWorldGenerator* WorldGenerator);
	AMCharacter* LoadMCharacter_Internal(const FMCharacterSaveData& MCharacterSD, AMWorldGenerator* WorldGenerator);

	void ClearMActorSD(const FMUid& Uid, const FIntPoint& BlockIndex);
	void ClearMCharacterSD(const FMUid& Uid, const FIntPoint& BlockIndex);

	TMap<FString, FComponentSaveData> GetSaveDataForComponents(AActor* Actor);
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
};

