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
	//TODO: We should split save into several files. Each will be responsible for a large chunk of the world, e.g. 100000 by 100000
	//TODO: I.e. to store TMap<FIntPoint, USaveGameWorld*> LoadedWorldRegions
	void SaveToMemory(FLRUCache& GridOfActors, const AMWorldGenerator* WorldGenerator);
	bool LoadFromMemory(AMWorldGenerator* WorldGenerator);

private:

	void LoadPerTick(AMWorldGenerator* WorldGenerator);
	void LoadBlock(const FIntPoint& BlockIndex, const FBlockSaveData& BlockSD, AMWorldGenerator* WorldGenerator);
	AMActor* LoadMActor(const FMActorSaveData& MActorSD, AMWorldGenerator* WorldGenerator);
	AMPickableActor* LoadMPickableActor(const FMPickableActorSaveData& MPickableActorSD, AMWorldGenerator* WorldGenerator);
	AMCharacter* LoadMCharacter(const FMCharacterSaveData& MCharacterSD, AMWorldGenerator* WorldGenerator);

	FTimerHandle AutoSavesTimer;
	FTimerHandle BlockLoadingTimer;

	UPROPERTY()
	USaveGameWorld* LoadedGameWorld;
};

