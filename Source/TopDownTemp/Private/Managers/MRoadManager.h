#pragma once

#include "CoreMinimal.h"
#include "MRoadManagerTypes.h"
#include "Math/UnrealMathUtility.h"
#include "Helpers/MGroundMarker.h"
#include "MRoadManager.generated.h"

class URoadManagerSave;
class AMRoadSplineActor;
class AMWorldGenerator;
class AMOutpostGenerator;

/** Class responsible for road generation within Regions, their Chunks and their blocks.\n\n
 *  Handles spawn of Outposts such as villages/camps/sites.\n\n
 *  Responsible for in-game navigation, direction signs, etc. */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API UMRoadManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AMWorldGenerator* IN_WorldGenerator);

	void ConnectTwoChunks(FIntPoint ChunkA, FIntPoint ChunkB, const ERoadType RoadType = ERoadType::MainRoad);

	void ConnectTwoBlocks(const FIntPoint& BlockA, const FIntPoint& BlockB, const ERoadType RoadType = ERoadType::Trail);

	/** If current chunk is near the edge of the current region, generate the roads for adjacent one(s) */
	void ProcessAdjacentRegions(const FIntPoint& CurrentChunk);

	void ProcessRegionIfUnprocessed(const FIntPoint& CurrentChunk);

	FIntPoint GetChunkSize() const { return ChunkSize; }

	FIntPoint GetRegionSize() const { return RegionSize; }

	FIntPoint GetChunkIndexByLocation(const FVector& Location) const;

	FIntPoint GetChunkIndexByBlock(const FIntPoint& BlockIndex) const;

	FIntPoint GetBlockIndexByChunk(const FIntPoint& ChunkIndex) const { return ChunkIndex * ChunkSize; }

	/** Returns the index of the block closest to the center of this chunk (rounded down) */
	FIntPoint GetChunkCenterBlock(const FIntPoint& ChunkIndex) const;

	FIntPoint GetRegionIndexByChunk(const FIntPoint& ChunkIndex) const;

	FIntPoint GetChunkIndexByRegion(const FIntPoint& RegionIndex) const { return RegionIndex * RegionSize; }

	UFUNCTION()
	void OnPlayerChangedChunk(const FIntPoint& OldChunk, const FIntPoint& NewChunk);

	/** The tag PCG uses to differ the road types */
	FName GetRoadPCGTag(ERoadType RoadType) const;

	const TMap<FName, TSubclassOf<AActor>>& GetOutpostBPClasses() const { return OutpostBPClasses; }

	void SaveToMemory();

	UMGroundMarker* GetGroundMarker() const { return GroundMarker; }

protected:

	/** If adjacent chunks have outpost generators, call their Generate() to spawn their actors */
	void TriggerOutpostGenerationForAdjacentChunks(const FIntPoint& CurrentChunk);

	void LoadOrGenerateRegion(const FIntPoint& RegionIndex);
	bool LoadConnectionsBetweenChunksWithinRegion(const FIntPoint& RegionIndex);
	void GenerateConnectionsBetweenChunksWithinRegion(const FIntPoint& RegionIndex);

	/** Spawns only the generator actor. The actual Generate() call will be triggered as soon as player enters the chunk or adjacent to it.\n
	 * @param Class Outpost generator class. If null, it will be selected randomly */
	void SpawnOutpostGenerator(const FIntPoint& Chunk, TSubclassOf<AMOutpostGenerator> Class = nullptr);

	UFUNCTION(BlueprintCallable)
	TMap<FUnorderedConnection, AMRoadSplineActor*>& GetRoadContainer(ERoadType RoadType);

	/** Chunk is a rectangle (commonly square) area consisting of adjacent ground blocks. Serves only geometry purposes.\n
	 * Roads go along chunk edges.\n
	 * Main requirement: shouldn't be smaller than the visible area (ActiveZoneRadius) */
	UPROPERTY(EditDefaultsOnly, Category="MRoadManager|Configuration")
	FIntPoint ChunkSize = {8, 8};

	// [] | [] | [] <----- chunk
	// -  + -  + -  <----- road 
	// [] | [] | []              <----- Region (2D area of chunks)
	// -  + -  + - 
	// [] | [] | []
	/** Area measured in chunks. The largest abstraction in generation logic.\n
	 *  Chunks' centers are the only possible places for structures (collections of buildings).\n
	 *  Roads run along the boundaries of the chunks. */
	UPROPERTY(EditDefaultsOnly, meta=(ClampMin="1", ClampMax="6"))
	FIntPoint RegionSize = {5, 5};

	/** Not all blocks are connected/have a road.\n
	 * This value determines the chance of being connected, and therefore having a structure on it */
	UPROPERTY(EditDefaultsOnly, meta=(ClampMin="0.1", ClampMax="1"))
	float ConnectionChance = 0.2f;

	/** How curve roads get */
	UPROPERTY(EditDefaultsOnly, Category="MRoadManager|Configuration", meta=(ClampMin="0.1", ClampMax="0.5"))
	float CurveFactor = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FName, TSubclassOf<AActor>> OutpostBPClasses;

private:
	/** It maps pairs of CHUNKS and corresponding spline actors (roads) between them.\n
	 *  Run only along chunk edges, usually large are part of long paths.\n
	 *  DON'T USE ANYWHERE EXCEPT GetRoadContainer()\n
	 */
	UPROPERTY()
	TMap<FUnorderedConnection, AMRoadSplineActor*> MainRoads;

	/** It maps pairs of BLOCKS and corresponding spline actors (trails) between them.\n
	 *  Lie between blocks, can have any shape, usually small and should be within one or two chunks.\n
	 *  It's impossible to track intersections between two trails.\n
	 *  DON'T USE ANYWHERE EXCEPT GetRoadContainer()\n
	 */
	UPROPERTY()
	TMap<FUnorderedConnection, AMRoadSplineActor*> Trails;

	/** Filled in only to create new ones or load existing ones needed for the current session. */
	UPROPERTY()
	TMap<FIntPoint, FChunkMetadata> GridOfChunks;

	/** Filled in only to create new ones or load existing ones needed for the current session. */
	UPROPERTY()
	TMap<FIntPoint, FRegionMetadata> GridOfRegions;
	
	UPROPERTY()
	URoadManagerSave* LoadedSave;

	//TODO: Remove it from here. It's a temporary measure for quicker prototyping
	UPROPERTY()
	AMWorldGenerator* pWorldGenerator;

private: // For debugging

	// Regions that are currently adjacent to the player. For debugging purposes only
	TSet<FIntPoint> AdjacentRegions;

	friend class UMGroundMarker;

	UPROPERTY()
	UMGroundMarker* GroundMarker;
};
