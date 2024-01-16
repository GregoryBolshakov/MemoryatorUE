#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MWorldGeneratorTypes.h"
#include "Components/MInventoryComponent.h"
#include "MSaveTypes.generated.h"

/** Saved actors might depend on other saved actors when loading. They use this ID. \n
 * Has no relation to the overall uniqueness of actors in the world */
USTRUCT()
struct FUid
{
	GENERATED_BODY()

	/** When the game is saved, only a portion of the objects touched by the player are written to memory.\n
	 * Intact objects remain unchanged from previous sessions, and therefore intersection of unique identifiers is possible.\n
	 * (Because NumberUniqueIndex resets every time game launches)\n
	 * This ID makes all objects unique within different launches */
	UPROPERTY()
	int32 LaunchId = MIN_int32;

	/** Unique object identifier within this game launch */
	UPROPERTY()
	int32 ObjectId = MIN_int32;

	friend bool operator==(const FUid& lhs, const FUid& rhs)
	{
		return lhs.LaunchId == rhs.LaunchId && lhs.ObjectId == rhs.ObjectId;
	}

	friend uint32 GetTypeHash(const FUid& uid)
	{
		return HashCombine(::GetTypeHash(uid.LaunchId), ::GetTypeHash(uid.ObjectId));
	}
};

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

	/** Saved actors might depend on other saved actors when loading. They use this ID.\n
	 * Has no relation to the overall uniqueness of actors in the world
	 * Grants total uniqueness for saved object among different game launches */
	UPROPERTY()
	FUid SavedUid;
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
	FUid HouseUid;
};

USTRUCT()
struct FBlockSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FPCGVariables PCGVariables;

	UPROPERTY()
	bool WasConstant = false;

	UPROPERTY()
	TArray<FMActorSaveData> SavedMActors;

	UPROPERTY()
	TArray<FMCharacterSaveData> SavedMCharacters;
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

	UPROPERTY()
	int32 LaunchId = MAX_int32;

	// We don't save ActorsMetadata for 2 reasons:
	// 1. Too large data. Very long read/write
	// 2. It is possible to recreate it reading SavedGrid data
	//
	// Once a world is loaded, ActorsMetadata is not immediately available. It loads in parallel
};
