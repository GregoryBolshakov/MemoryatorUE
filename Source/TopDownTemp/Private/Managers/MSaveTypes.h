#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MWorldGeneratorTypes.h"
#include "Components/MInventoryComponent.h"
#include "MSaveTypes.generated.h"

USTRUCT()
struct FActorSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TSubclassOf<AActor> FinalClass;

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FString NameString;
};

USTRUCT()
struct FMActorSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FActorSaveData ActorSaveData;

	UPROPERTY()
	int AppearanceID;

	UPROPERTY()
	bool IsRandomizedAppearance;
};

USTRUCT()
struct FMPickableActorSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FMActorSaveData MActorSaveData;

	UPROPERTY()
	TArray<FItem> InventoryContents;
};

USTRUCT()
struct FMCharacterSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FActorSaveData ActorSaveData;

	UPROPERTY()
	FName SpeciesName;

	UPROPERTY()
	float Health;
};

USTRUCT()
struct FBlockSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	EBiome Biome;

	UPROPERTY()
	TArray<FMActorSaveData> SavedMActors;

	UPROPERTY()
	TArray<FMCharacterSaveData> SavedMCharacters;

	UPROPERTY()
	TArray<FMPickableActorSaveData> SavedMPickableActors;
};

UCLASS()
class USaveGameWorld : public USaveGame
{
	GENERATED_BODY()

public:
	inline static FString SlotName {"WorldSave"};

	/** Used as a deque, this represents the sequence of blocks traveled by the player.
	 * The newest blocks are appended to the end, while the oldest ones are removed from the beginning when the limit is reached */
	UPROPERTY()
	TArray<FIntPoint> PlayerTraveledPath;

	/** Saved blocks from UMWorldGenerator.GridOfActors */
	UPROPERTY()
	TMap<FIntPoint, FBlockSaveData> SavedGrid;

	/** Order for blocks to be generated */
	UPROPERTY()
	TArray<FIntPoint> GridOrder;

	// We don't save ActorsMetadata for 2 reasons:
	// 1. Too large data. Very long read/write
	// 2. It is possible to recreate it reading SavedGrid data
	//
	// Once a world is loaded, ActorsMetadata is not immediately available. It loads in parallel
};
