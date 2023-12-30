#pragma once

#include "CoreMinimal.h"
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
struct FMPickableActorSaveData;
struct FMCharacterSaveData;

DECLARE_LOG_CATEGORY_EXTERN(LogSaveManager, Log, All);

UCLASS()
class UMSaveManager : public UObject
{
	GENERATED_BODY()

public:

	void SetUpAutoSaves(FLRUCache& GridOfActors, const AMWorldGenerator* WorldGenerator);
	void SaveToMemory(FLRUCache& GridOfActors, const AMWorldGenerator* WorldGenerator);
	bool LoadFromMemory();

private:

	void LoadPerTick(AMWorldGenerator* WorldGenerator);
	void LoadBlock(const FIntPoint& BlockIndex, const FBlockSaveData& BlockSD, AMWorldGenerator* WorldGenerator);
	AMActor* LoadMActor(const FMActorSaveData& MActorSD, AMWorldGenerator* WorldGenerator);
	AMPickableActor* LoadMPickableActor(const FMPickableActorSaveData& MPickableActorSD, AMWorldGenerator* WorldGenerator);
	AMCharacter* LoadMCharacter(const FMCharacterSaveData& MCharacterSD, AMWorldGenerator* WorldGenerator);

	FTimerHandle AutoSavesTimer;
	FTimerHandle BlockLoadingTimer;

	//TODO: To support truly endless worlds we should split save into several files.
	//TODO: Each will be responsible for a large chunk/region of the world, e.g. 1000 by 1000 blocks
	//TODO: I.e. we're gonna store saved regions in TMap<FIntPoint, USaveGameWorld*> LoadedWorldRegions
	//TODO: When player enters a region, mark it as dirty
	//TODO: When Saving, iterate all dirty/marked regions, save them and reset their dirty/marked state
	UPROPERTY()
	USaveGameWorld* LoadedGameWorld;
};

