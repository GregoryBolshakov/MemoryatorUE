#pragma once

#include "CoreMinimal.h"
#include "ReplicationGraph.h"
#include "MReplicationGraph.generated.h"

class UReplicationGraphNode_GridSpatialization2D;
class UReplicationGraphNode_ActorList;

enum class EClassRepPolicy
{
	NotRouted,
	RelevantAllConnections,

	// Spatialized routes into the grid node
	Spatialize_Static,
	Spatialize_Dynamic,
	Spatialize_Dormancy
};

/**  */
UCLASS(Transient, config=Engine)
class UMReplicationGraph : public UReplicationGraph
{
	GENERATED_BODY()

	// ~ begin UReplicationGraph implementation
	virtual void ResetGameWorldState() override;
	virtual void InitConnectionGraphNodes(UNetReplicationGraphConnection* ConnectionManager) override;
	virtual void InitGlobalActorClassSettings() override;
	virtual void InitGlobalGraphNodes() override;
	virtual void RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo) override;
	virtual void RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo) override;
	// ~ end UReplicationGraph

	/** Sets class replication info for a class */
	void InitClassReplicationInfo(FClassReplicationInfo& Info, UClass* Class, bool bSpatialize, float ServerMaxTickRate);

	TArray<UClass*> SpatializedClasses;
	TArray<UClass*> NonSpatializedClasses;
	TArray<UClass*> AlwaysRelevantClasses;

	// Replication graph nodes

	/**
	 * This is probably the most important node in the replication graph
	 * 
	 * Carves the map up into grids and determines if an actor should send network updates 
	 * to a connection depending on the different pre-defined grids
	 */
	UPROPERTY()
	UReplicationGraphNode_GridSpatialization2D* GridNode;

	UPROPERTY()
	UReplicationGraphNode_ActorList* AlwaysRelevantNode;

public:
	/** Maps the actors that need to be always relevant across streaming levels */
	TMap<FName, FActorRepListRefView> AlwaysRelevantStreamingLevelActors;

protected:

	FORCEINLINE bool IsSpatialized(EClassRepPolicy Policy) { return Policy >= EClassRepPolicy::Spatialize_Static; }

	TClassMap<EClassRepPolicy> RepPoliciesClassMap;
	// Gets mapping by class
	EClassRepPolicy GetMappingPolicy(UClass* IN_Class);
	
	float GridCellSize = 10000.f;						// The size of one grid cell in the grid node
	FVector2D SpatialBias = {-150000.f, -200000.f};	// "Min X and Y" for replication
	bool bDisableSpatialRebuilding = true;
};

UCLASS()
class UMReplicationGraphNode_AlwaysRelevant_ForConnection : public UReplicationGraphNode_AlwaysRelevant_ForConnection
{
public:
	GENERATED_BODY()

	// ~ begin UReplicationGraphNode_AlwaysRelevant_ForConnection implementation
	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters& Params) override;
	// ~ end UReplicationGraphNode_AlwaysRelevant_ForConnection implementation

	void OnClientLevelVisibilityAdd(FName LevelName, UWorld* LevelWorld);
	void OnClientLevelVisibilityRemove(FName LevelName);

	void ResetGameWorldState();

protected:
	/** Stores level streaming actors */
	TArray<FName, TInlineAllocator<64>> AlwaysRelevantStreamingLevels;
};