#pragma once

#include "CoreMinimal.h"
#include "Math/UnrealMathUtility.h"
#include "MRoadManager.generated.h"

class AMRoadSplineActor;
class AMWorldGenerator;

USTRUCT(BlueprintType)
struct FUnorderedConnection
{
	GENERATED_BODY()
	FIntPoint A;
	FIntPoint B;

	bool operator==(const FUnorderedConnection& Other) const
	{
		return (A == Other.A && B == Other.B) ||
			   (A == Other.B && B == Other.A);
	}
};

namespace FUnorderedConnectionHash
{
	FORCEINLINE uint32 HashCombine(uint32 A, uint32 B)
	{
		return A ^ (B + 0x9e3779b9 + (A << 6) + (A >> 2));
	}
}

USTRUCT()
struct FChunkMetadata
{
	GENERATED_BODY()
	bool bProcessed = false;
};

USTRUCT()
struct FRegionMetadata
{
	GENERATED_BODY()
	bool bProcessed = false;
};

FORCEINLINE uint32 GetTypeHash(const FUnorderedConnection& Connection)
{
	uint32 HashA = GetTypeHash(Connection.A);
	uint32 HashB = GetTypeHash(Connection.B);

	// Combine hashes in a way that is order-independent
	return FUnorderedConnectionHash::HashCombine(FMath::Min(HashA, HashB), FMath::Max(HashA, HashB));
}

UCLASS(Blueprintable)
class TOPDOWNTEMP_API UMRoadManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AMWorldGenerator* IN_WorldGenerator) { pWorldGenerator = IN_WorldGenerator; }

	void ConnectTwoChunks(const FIntPoint& ChunkA, const FIntPoint& ChunkB);

	UFUNCTION()
	void ConnectChunksWithinRegion(/*const FIntPoint& Center (OLD)*/ const FIntPoint& RegionIndex);

	FIntPoint GetChunkIndexByLocation(const FVector& Location) const;

	FIntPoint GetChunkIndexByBlock(const FIntPoint& BlockIndex) const;

	FIntPoint GetBlockIndexByChunk(const FIntPoint& ChunkIndex) const { return ChunkIndex * ChunkSize; }

	FIntPoint GetRegionIndexByChunk(const FIntPoint& ChunkIndex) const;

	FIntPoint GetChunkIndexByRegion(const FIntPoint& RegionIndex) const { return RegionIndex * RegionSize; }

	UFUNCTION()
	void OnPlayerChangedChunk(const FIntPoint& OldChunk, const FIntPoint& NewChunk);

protected:
	/** Chunk is a rectangle (commonly square) area consisting of adjacent ground blocks. Serves only geometry purposes. Roads go along chunk edges */
	UPROPERTY(EditDefaultsOnly, Category="MRoadManager|Configuration")
	FIntPoint ChunkSize = {8, 8};

	// [] | [] | [] <----- chunk
	// -  + -  + -  <----- road 
	// [] | [] | []              <----- Region (2D area of chunks)
	// -  + -  + - 
	// [] | [] | []
	/** Area measured in chunks. The largest abstraction in generation logic.
	 *  Chunks' centers are the only possible places for structures (collections of buildings).
	 *  Roads run along the boundaries of the chunks. */
	UPROPERTY(EditDefaultsOnly, meta=(ClampMin="1", ClampMax="6"))
	FIntPoint RegionSize = {5, 5};

	/** Not all blocks are connected/have a road.
	 * This value determines the chance of being connected, and therefore having a structure on it */
	UPROPERTY(EditDefaultsOnly, meta=(ClampMin="0.1", ClampMax="1"))
	float ConnectionChance = 0.2f;

	/** How curve roads get */
	UPROPERTY(EditDefaultsOnly, Category="MRoadManager|Configuration", meta=(ClampMin="0.1", ClampMax="0.5"))
	float CurveFactor = 0.35f;

	/** It maps pairs of CHUNKS and corresponding spline actors (roads) between them
	 *  Run only along chunk edges, usually large are part of long paths
	 */
	UPROPERTY()
	TMap<FUnorderedConnection, AMRoadSplineActor*> Roads;

	/** It maps pairs of BLOCKS and corresponding spline actors (roads) between them
	 *  Lie between blocks, can have any shape, usually small and should be within one or two chunks.
	 *  It's impossible to track intersections between two trails.
	 */
	UPROPERTY()
	TMap<FUnorderedConnection, AMRoadSplineActor*> Trails;

private:
	UPROPERTY()
	TMap<FIntPoint, FChunkMetadata> GridOfChunks;

	UPROPERTY()
	TMap<FIntPoint, FRegionMetadata> GridOfRegions;

	//TODO: Remove it from here. It's a temporary measure for quicker prototyping
	UPROPERTY()
	AMWorldGenerator* pWorldGenerator;
};

