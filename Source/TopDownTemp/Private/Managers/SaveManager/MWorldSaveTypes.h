#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Managers/MWorldGeneratorTypes.h"
#include "Components/MInventoryComponent.h"
#include "MWorldSaveTypes.generated.h"

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
	FMUid Uid;
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

	UPROPERTY()
	TArray<FItem> InventoryContents;

	/** Uid of the house where this character resides. MIN_int32 means not assigned */
	UPROPERTY()
	FMUid HouseUid;
};

USTRUCT()
struct FBlockSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FPCGVariables PCGVariables;

	UPROPERTY()
	int ConstantActorsCount = 0;

	UPROPERTY()
	TMap<FMUid, FMActorSaveData> SavedMActors;

	UPROPERTY()
	TMap<FMUid, FMCharacterSaveData> SavedMCharacters;
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

	/** Saved blocks from GridOfActors */
	UPROPERTY()
	TMap<FIntPoint, FBlockSaveData> SavedGrid;

	UPROPERTY()
	int32 LaunchId = MAX_int32;

	// We don't save ActorsMetadata for 2 reasons:
	// 1. Too large data. Very long read/write
	// 2. It is possible to recreate it reading SavedGrid data
	//
	// Once a world is loaded, ActorsMetadata is not immediately available. It loads in parallel
};
