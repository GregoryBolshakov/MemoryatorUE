#pragma once

#include "CoreMinimal.h"
#include "MWorldSaveTypes.h"
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

inline bool IsUidValid(const FUid& Uid) { return Uid.ObjectId > MIN_int32; }

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
	const FMActorSaveData* GetMActorData(const FUid& Uid);
	const FMCharacterSaveData* GetMCharacterData(const FUid& Uid);
	void RemoveBlock(const FIntPoint& Index);
	TArray<FIntPoint> GetPlayerTraveledPath() const;
	bool IsLoaded() const { return LoadedGameWorld != nullptr; }

	/** Needed for actors to precisely load dependant actors. It will remove the save data from SaveManager */
	AMActor* LoadMActorAndClearSD(const FUid& Uid, AMWorldGenerator* WorldGenerator);
	/** Needed for actors to precisely load dependant characters. It will remove the save data from SaveManager */
	AMCharacter* LoadMCharacterAndClearSD(const FUid& Uid, AMWorldGenerator* WorldGenerator);

	const TMap<FName, FUid>& GetNameToUidMap() const { return NameToUidMap; }

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

private: // Relations between saved actors
	/** Matches FUid with pointers to FMActorSaveData stored in all blocks in the LoadedGameWorld.*/
	TMap<FUid, FMActorSaveData*> LoadedMActorMap;

	/** Matches FUid with pointers to FMCharacterSaveData stored in all blocks in the LoadedGameWorld.*/
	TMap<FUid, FMCharacterSaveData*> LoadedMCharacterMap;

	/** Matches the names of actors spawned during this session with their Uid. Filled only during SaveToMemory. */
	UPROPERTY()
	TMap<FName, FUid> NameToUidMap;

	UPROPERTY()
	TMap<FUid, AActor*> AlreadySpawnedSavedActors;
	// FUid нужен всем актерам в мире как только они спавнятся.
	// Сейчас проблема при SaveToMemory(): ссылаясь на FUid актера, не факт, что он был сгенерирован.
	// А сейчас он генерируется только при сохранении актера
	// Есть 2 проблемы:
	// 1) Актеры много раз удаляются и не доживают до сейва,
	// но можно пренебречь, т.к. количества уникальных идентификаторов хватит на ~23860 часов сессии, если в секунду создавать по 50 актеров.
	// 2) Разные классы актеров AMActor, AMCharacter, возможно AMOutpostGenerator. Всем придется добавлять Fuid отдельно.
};

