#pragma once

#include "CoreMinimal.h"
#include "SaveManager/MUid.h"
#include "MWorldGeneratorTypes.generated.h"

class AMPlayerController;
class UPCGGraphInterface;
class UPCGGraph;
class AMRoadSplineActor;
class ASplineMeshActor;
class AMGroundBlock;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBlockChanged, const FIntPoint&, OldBlock, const FIntPoint&, NewBlock, const AMPlayerController*, PlayerController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnChunkChanged, const FIntPoint&, OldChunk, const FIntPoint&, NewChunk, const uint8, ObserverIndex); // TODO: Replace to player controller
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

USTRUCT()
struct FObserverFlags
{
	GENERATED_BODY()

	FORCEINLINE void SetBit(uint8 BitIndex) {
		// Atomically set the bit to 1
		FPlatformAtomics::InterlockedOr((int32*)&Flags, 1 << BitIndex);
	}

	FORCEINLINE bool CheckBit(uint8 BitIndex) const
	{
		// Atomically check the bit
		return Flags.Load() & (1 << BitIndex);
	}

	FORCEINLINE void ClearBit(uint8 BitIndex) {
		// Atomically clear the bit
		FPlatformAtomics::InterlockedAnd((int32*)&Flags, ~(1 << BitIndex));
	}

	FORCEINLINE bool IsEmpty() const
	{
		// Check if any bit is set
		return Flags.Load() == 0;
	}

	FORCEINLINE bool IsAnyOtherBitSet(uint8 BitIndex) const
	{
		// Check if any other bit is set, excluding the bit at BitIndex
		uint32 mask = ~(1 << BitIndex);
		return (Flags.Load() & mask) != 0;
	}

	FObserverFlags() {}

	// Copy constructor
	FObserverFlags(const FObserverFlags& Other)
	{
		Flags.Store(Other.Flags.Load());
	}

	// Copy assignment operator
	FObserverFlags& operator=(const FObserverFlags& Other)
	{
		if (this != &Other)
		{
			Flags.Store(Other.Flags.Load());
		}
		return *this;
	}

	TAtomic<uint32> Flags;
};

/** Is needed for pool of blocks waiting to be generated. Each block needs to know which player it is generated for */
struct FBlockAndObserver
{
	FIntPoint BlockIndex;
	uint8 ObserverIndex = 0;

	bool operator==(const FBlockAndObserver& Other) const
	{
		// If multiple players trigger a block generation, generate only for the first one
		return BlockIndex == Other.BlockIndex;
	}
};

/** Hash function for FBlockAndObserver */
FORCEINLINE uint32 GetTypeHash(const FBlockAndObserver& BlockAndObserver)
{
	return GetTypeHash(BlockAndObserver.BlockIndex);
}

/** Class for storing the data needed for world generation per grid block */
UCLASS()
class UBlockMetadata : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TMap<FName, AActor*> StaticActors;

	UPROPERTY()
	TMap<FName, AActor*> DynamicActors;

	/** If there is at least one AlwaysEnabled actor on the block, it won't be regenerated */
	UPROPERTY()
	int ConstantActorsCount = 0;

	/** Multiplayer limit is 32 players. Each block knows whether a certain player is observing it.\n
	 * Each player is given a unique flag bit position when entering the game. */
	UPROPERTY()
	FObserverFlags ObserverFlags;

	/** Is set in either AMWorldGenerator::PrepareVisibleZone, or UMSaveManager::LoadBlock */
	UPROPERTY()
	EBiome Biome;

	/** Explicitly exposed to avoid frequent lookup in StaticActors */
	UPROPERTY()
	AMGroundBlock* pGroundBlock = nullptr;

	UPROPERTY()
	UPCGGraph* PCGGraph = nullptr;
};

USTRUCT()
struct FLRUCache
{
	GENERATED_BODY()

	UBlockMetadata* Get(const FIntPoint& Key)
	{
		if (DataMap.Contains(Key))
		{
			// Move accessed key to front
			CacheOrder.RemoveSingleSwap(Key); // This makes the concept not pure
			CacheOrder.Insert(Key, 0);
			return DataMap[Key];
		}
		return nullptr;
	}

	UBlockMetadata* Add(const FIntPoint& Key, UBlockMetadata* Value)
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
	TMap<FIntPoint, UBlockMetadata*> DataMap;

	TArray<FIntPoint> CacheOrder;
	int32 Capacity = 1e7;
};

/** Utility class for storing actor's metadata in the grid */
UCLASS()
class UActorWorldMetadata : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	AActor* Actor;

	/** Saved actors might depend on other saved actors when loading. They use this ID.\n
	*   Provides the overall uniqueness of actors in the world
	*   Grants total uniqueness across different game launches */
	UPROPERTY(BlueprintReadOnly)
	FMUid Uid;

	FIntPoint GroundBlockIndex;

	FOnBlockChanged OnBlockChangedDelegate;

	FOnChunkChanged OnChunkChangedDelegate;
};

enum class EScreenPoint
{
	TopLeft = 0,
	TopRight
};

/** Struct for storing all PCG information for a block. E.g. Biome, Graph, amount of trees, bushes, etc.\n
 * Replicated. On replication triggers block generation for the owning block.\n
 * Is set/modified only once, right after the block is spawned on the Server.*/
USTRUCT(BlueprintType)
struct FPCGVariables
{
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UPCGGraphInterface> Graph = nullptr;
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EBiome Biome;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int TreesCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int BushesCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int StonesCount = 0;
};
