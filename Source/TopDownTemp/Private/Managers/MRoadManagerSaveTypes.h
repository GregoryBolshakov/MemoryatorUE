#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MRoadManagerTypes.h"
#include "MWorldSaveTypes.h"
#include "MRoadManagerSaveTypes.generated.h"

USTRUCT()
struct FChunkSaveData
{
	GENERATED_BODY()
	bool bConnectedOrIgnored = false;

	/** Currently we support only up to one Outpost, e.g. a village/camp/site per chunk */
	//UPROPERTY()
	//FOutpostSaveData OutpostSaveData;
	UPROPERTY()
	FUid OutpostUid;
};

/*USTRUCT()
struct FOutpostSaveData
{
	GENERATED_BODY()
	FUid Uid;
};*/

/** Here we store all the saved data of a Region. We only save already PROCESSED regions! */
USTRUCT()
struct FRegionSaveData
{
	GENERATED_BODY()
};

UCLASS()
class URoadManagerSave : public USaveGame
{
	GENERATED_BODY()

public:
	inline static FString SlotName {"RoadManagerSave"};

	UPROPERTY()
	TMap<FIntPoint, FRegionSaveData> SavedRegions;

	UPROPERTY()
	TMap<FIntPoint, FChunkSaveData> SavedChunks;

	UPROPERTY()
	TSet<FUnorderedConnection> MainRoads;

	UPROPERTY()
	TSet<FUnorderedConnection> Trails;
};
