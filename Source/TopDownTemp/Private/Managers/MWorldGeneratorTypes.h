#pragma once

#include "CoreMinimal.h"
#include "MWorldGeneratorTypes.generated.h"

class AMGroundBlock;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBlockChanged, const FIntPoint&, OldBlock, const FIntPoint&, NewBlock);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpawnActorStarted, AActor*);

UENUM(BlueprintType)
enum class EBiome : uint8
{
	DarkWoods = 0,
	BirchGrove,
	Swamp
};

/** Used when generating random biomes coloring for generation perimeter */
USTRUCT()
struct FBiomeDelimiter
{
	GENERATED_BODY()
	int BlockPosition;
	EBiome Biome;
};

/** Class for storing actors within one block of the frid */
UCLASS()
class UBlockOfActors : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TMap<FName, AActor*> StaticActors;

	UPROPERTY()
	TMap<FName, AActor*> DynamicActors;

	//bool IsConstant = false; //TODO: now it is not evaluated! We should track EnrollToGrid() and RemoveFromGrid() for AlwaysEnabled actors
	/** If there is at least one AlwaysEnabled actor on the block, it won't be regenerated */
	UPROPERTY()
	int ConstantActorsCount = 0;

	/** Is set in either AMWorldGenerator::PrepareVisibleZone, or UMSaveManager::LoadBlock */
	UPROPERTY()
	EBiome Biome;

	//TODO: Come up how not to store it here. Temp workaround for ground block transitions
	UPROPERTY()
	AMGroundBlock* pGroundBlock = nullptr;
};

USTRUCT()
struct FLRUCache
{
	GENERATED_BODY()

	UBlockOfActors* Get(const FIntPoint& Key)
	{
		if (DataMap.Contains(Key))
		{
			// Move accessed key to front
			CacheOrder.RemoveSingleSwap(Key);
			CacheOrder.Insert(Key, 0);
			return DataMap[Key];
		}
		return nullptr;
	}

	UBlockOfActors* Add(const FIntPoint& Key, UBlockOfActors* Value)
	{
		// If already in cache, we'll be updating the order next
		if (!DataMap.Contains(Key))
		{
			if (CacheOrder.Num() >= Capacity)
			{
				// Evict least recently accessed item
				FIntPoint LastKey = CacheOrder.Pop();
				DataMap.Remove(LastKey);
			}
		}

		// Add or update the item in the map
		DataMap.Add(Key, Value);

		// Move (or add) key to front of order
		CacheOrder.RemoveSingleSwap(Key);
		CacheOrder.Insert(Key, 0);

		// Return a pointer to the item in the map
		return DataMap[Key];
	}

	int32 Num() const
	{
		return CacheOrder.Num();
	}

	TArray<FIntPoint> GetCacheOrder() const { return CacheOrder; }

private:
	UPROPERTY()
	TMap<FIntPoint, UBlockOfActors*> DataMap;

	TArray<FIntPoint> CacheOrder;
	int32 Capacity = 1e7;
};

/** Utility class for storing actor's metadata in the grid */
USTRUCT()
struct FActorWorldMetadata
{
	GENERATED_BODY()
public:
	UPROPERTY()
	AActor* Actor;

	FIntPoint GroundBlockIndex;

	FOnBlockChanged OnBlockChangedDelegate;
};

enum class EScreenPoint
{
	TopLeft = 0,
	TopRight
};
