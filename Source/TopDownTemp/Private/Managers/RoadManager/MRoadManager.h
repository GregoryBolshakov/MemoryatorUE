#pragma once

#include "CoreMinimal.h"
#include "MRoadManagerTypes.h"
#include "Math/UnrealMathUtility.h"
#include "Helpers/MGroundMarker.h"
#include "MRoadManager.generated.h"

class UMRoadMap;
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

	const TSet<FIntPoint> GetAdjacentRegions(const FIntPoint& ChunkIndex) const;

	/** Set the observer flag for the current region and all adjacent regions to the chunk.
	 * It's not the same as all regions adjacent to the current region! Only to the chunk. */
	void AddObserverToZone(const FIntPoint& ChunkIndex, const uint8 ObserverIndex);

	/** Helper function. Called for each region by AddObserverToZone() and MoveObserverToZone() */
	void AddObserverToRegion(const FIntPoint& RegionIndex, const uint8 ObserverIndex);

	// TODO: Implement void RemoveObserverFromZone(const FIntPoint& ChunkIndex, const uint8 ObserverIndex);
	// It should be unloading regions that are no longer adjacent to anybody

	/** Same as AddObserverToChunk but also removes the Observer index from regions left by the observer.
	 * Need it to avoid instant -1 +1 problem. */
	void MoveObserverToZone(const FIntPoint& PreviousChunk, const FIntPoint& NewChunk, const uint8 ObserverIndex);

	//TODO: Save not adjacent regions and remove them. It's optimizes performance and boosts saving time

	FIntPoint GetChunkSize() const { return ChunkSize; }

	FIntPoint GetRegionSize() const { return RegionSize; }

	FIntPoint GetChunkIndexByLocation(const FVector& Location) const;

	FIntPoint GetChunkIndexByBlock(const FIntPoint& BlockIndex) const;

	FIntPoint GetBlockIndexByChunk(const FIntPoint& ChunkIndex) const { return ChunkIndex * ChunkSize; }

	/** Returns the index of the block closest to the center of this chunk (rounded down) */
	FIntPoint GetChunkCenterBlock(const FIntPoint& ChunkIndex) const;

	FIntPoint GetRegionIndexByChunk(const FIntPoint& ChunkIndex) const;

	FIntPoint GetChunkIndexByRegion(const FIntPoint& RegionIndex) const { return RegionIndex * RegionSize; }

	/** Get the chunk's outpost. If the chunk has no outpost generated, return nullptr */
	UFUNCTION()
	AMOutpostGenerator* GetOutpostGenerator(const FIntPoint& ChunkIndex);

	UFUNCTION()
	void OnPlayerChangedChunk(const FIntPoint& OldChunk, const FIntPoint& NewChunk, const uint8 ObserverIndex);

	const TMap<FName, TSubclassOf<AActor>>& GetOutpostBPClasses() const { return OutpostBPClasses; }

	void SaveToMemory();

public: // For debugging
	UMGroundMarker* GetGroundMarker() const { return GroundMarker; }

	AMOutpostGenerator* SpawnOutpostGeneratorForDebugging(const FIntPoint& Chunk, TSubclassOf<AMOutpostGenerator> Class = nullptr);

protected: // Road navigation
	/** Adds BlockB to BlockA's connections and vice versa. */
	void AddConnection(const FIntPoint& BlockA, const FIntPoint& BlockB, AMRoadSplineActor* RoadActor);

	//TODO: RemoveConnection() ...

	/** Finds a road actor connecting two given blocks. Their order does not matter. */
	const AMRoadSplineActor* GetRoadActor(const FIntPoint& BlockA, const FIntPoint& BlockB, ERoadType RoadType);

	/** Finds all blocks connected to a given one. */
	FRoadActorMapWrapper GetConnections(const FIntPoint& Block, ERoadType RoadType);

protected:
	/** If adjacent chunks have outpost generators, call their Generate() to spawn their actors */
	void TriggerOutpostGenerationForAdjacentChunks(const FIntPoint& CurrentChunk);

	void LoadOrGenerateRegion(const FIntPoint& RegionIndex);
	bool LoadConnectionsBetweenChunksWithinRegion(const FIntPoint& RegionIndex); //TODO: Come up with better name
	void GenerateConnectionsBetweenChunksWithinRegion(const FIntPoint& RegionIndex); //TODO: Come up with better name

	/** Spawns only the generator actor. The actual Generate() call will be triggered as soon as player enters the chunk or adjacent to it.\n
	 * @param Class Outpost generator class. If null, it will be selected randomly */
	void SpawnOutpostGenerator(const FIntPoint& Chunk, TSubclassOf<AMOutpostGenerator> Class = nullptr);

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

	// Regions that are currently adjacent to the player (including the one the player is currently on). For debugging purposes only
	TSet<FIntPoint> AdjacentRegions;

	friend class UMGroundMarker;

	/** A class responsible for debug rendering of ground geometry */
	UPROPERTY()
	UMGroundMarker* GroundMarker;
};
