#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MRoadManagerTypes.h"
#include "Managers/MWorldSaveTypes.h"
#include "Managers/SaveManager/MUid.h"
#include "StationaryActors/MRoadSplineActor.h"
#include "MRoadManagerSaveTypes.generated.h"

USTRUCT()
struct FChunkSaveData
{
	GENERATED_BODY()
	bool bConnectedOrIgnored = false;

	//TODO: Create a separate save data for outposts
	UPROPERTY()
	FMUid OutpostUid;
};

//TODO: Implement save data for road actors here
USTRUCT()
struct FSavedMapWrapper
{
	/** Road actors, available only within the game session */
	GENERATED_BODY()
	UPROPERTY()
	TMap<FIntPoint, bool> Map; // TODO: Use road actors save data struct when it comes out
};
USTRUCT()
struct FSavedMatrixWrapper
{
	GENERATED_BODY()
	UPROPERTY()
	TMap<FIntPoint, FSavedMapWrapper> Matrix;
};

/** Here we store all the saved data of a Region. We only save already PROCESSED regions! */
USTRUCT()
struct FRegionSaveData
{
	GENERATED_BODY()
	/** Initializes save data using metadata */
	void Initialize(const FRegionMetadata& Metadata)
	{
		for (const auto& [ERoadType, MatrixWrapper] : Metadata.MatrixWrappers)
		{
			FSavedMatrixWrapper MatrixToSave;
			for (const auto& [IndexA, RoadActorMapWrapper] : MatrixWrapper.Matrix)
			{
				FSavedMapWrapper MapToSave;
				for (const auto& [IndexB, RoadActor] : RoadActorMapWrapper.Map)
				{
					MapToSave.Map.Add(IndexB, true); // TODO: Extract save data
				}
				MatrixToSave.Matrix.Add(IndexA, MapToSave);
			}
			SavedMatrices.Add(ERoadType, MatrixToSave);
		}
		/*TODO: if new fields are added to FRegionMetadata, cover them here*/
	}
	UPROPERTY()
	TMap<ERoadType, FSavedMatrixWrapper> SavedMatrices;
};

/** Main save struct for RoadManager */
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
};
