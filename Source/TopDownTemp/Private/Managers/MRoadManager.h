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

	/** Outdated functionality to endlessly continue roads which end on the outer perimeter */
	void GenerateNewPieceForRoads(const TSet<FIntPoint>& BlocksOnPerimeter);

	FIntPoint GetChunkIndexByLocation(const FVector& Location) const;

	FIntPoint GetBlockIndexByChunk(const FIntPoint& ChunkIndex) const { return ChunkIndex * ChunkSize; }

protected:
	/** Chunk is a rectangle (commonly square) area consisting of adjacent ground blocks. Serves only geometry purposes. Roads go along chunk edges */
	UPROPERTY(EditDefaultsOnly, Category="MRoadManager|Configuration")
	FIntPoint ChunkSize = {8, 8};

	/** How curve roads get */
	UPROPERTY(EditDefaultsOnly, Category="MRoadManager|Configuration", meta=(ClampMin="0.1", ClampMax="0.5"))
	float CurveFactor = 0.35f;

private:
	/** It maps pairs of blocks and corresponding spline actors (roads) between them */
	UPROPERTY()
	TMap<FUnorderedConnection, AMRoadSplineActor*> Roads;

	AMWorldGenerator* pWorldGenerator;
};

