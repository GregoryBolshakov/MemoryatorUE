#pragma once

#include "CoreMinimal.h"
#include "MRoadManagerTypes.generated.h"

class AMRoadSplineActor;
class AMOutpostGenerator;

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

UENUM(BlueprintType)
enum class ERoadType : uint8
{
	MainRoad = 0,
	Trail,
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(ERoadType, ERoadType::Count);

USTRUCT()
struct FChunkMetadata // TODO: Consider converting to a class
{
	GENERATED_BODY()
	/** The chunk was already either connected to another chunk, or decided to be ignored.
	* Either way it won't be able to pair with such another chunk. */
	bool bConnectedOrIgnored = false;

	/** Currently we support only up to one Outpost, e.g. a village/camp/site per chunk */
	UPROPERTY()
	AMOutpostGenerator* OutpostGenerator = nullptr;
};

USTRUCT()
struct FRoadActorMapWrapper
{
	/** Road actors, available only within the game session */
	GENERATED_BODY()
	UPROPERTY()
	TMap<FIntPoint, AMRoadSplineActor*> Map;
};
USTRUCT()
struct FRoadMatrixWrapper
{
	GENERATED_BODY()
	UPROPERTY()
	TMap<FIntPoint, FRoadActorMapWrapper> Matrix;
};

USTRUCT()
struct FRegionMetadata
{
	GENERATED_BODY()
	/** If the chunks within were divided into pairs and roads were spawned. */
	bool bProcessed = false;

	/** Matches road type with connectivity matrices */
	UPROPERTY()
	TMap<ERoadType, FRoadMatrixWrapper> MatrixWrappers;
};
