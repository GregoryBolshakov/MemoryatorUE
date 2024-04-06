#include "MReplicationGraph.h"
#include "StationaryActors/MActor.h"
#include "Characters/MCharacter.h"

void UMReplicationGraph::ResetGameWorldState()
{
	Super::ResetGameWorldState();
	AlwaysRelevantStreamingLevelActors.Empty();

	for (auto& ConnectionList : { Connections, PendingConnections })
	{
		for (UNetReplicationGraphConnection* Connection : ConnectionList)
		{
			for (UReplicationGraphNode* ConnectionNode : Connection->GetConnectionGraphNodes())
			{
				auto* Node = Cast<UMReplicationGraphNode_AlwaysRelevant_ForConnection>(ConnectionNode);
				if (Node)
				{
					Node->ResetGameWorldState();
				}
			}
		}
	}
}

void UMReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* ConnectionManager)
{
	auto* Node = CreateNewNode<UMReplicationGraphNode_AlwaysRelevant_ForConnection>();
	ConnectionManager->OnClientVisibleLevelNameAdd.AddUObject(Node, &UMReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityAdd);
	ConnectionManager->OnClientVisibleLevelNameRemove.AddUObject(Node, &UMReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityRemove);

	AddConnectionGraphNode(Node, ConnectionManager);
}

void UMReplicationGraph::InitGlobalActorClassSettings()
{
	Super::InitGlobalActorClassSettings();

	// Assign mapping to classes
	auto SetRule = [&](UClass* Class, EClassRepPolicy Policy) { RepPoliciesClassMap.Set(Class, Policy); };

	SetRule(AInfo::StaticClass(), EClassRepPolicy::RelevantAllConnections);
	SetRule(AMActor::StaticClass(), EClassRepPolicy::Spatialize_Static);
	SetRule(AMCharacter::StaticClass(), EClassRepPolicy::Spatialize_Dynamic);

	TArray<UClass*> ReplicatedClasses;
	for (TObjectIterator<UClass> Itr; Itr; ++Itr)
	{
		UClass* Class = *Itr;
		AActor* ActorCDO = Cast<AActor>(Class->GetDefaultObject());

		// Do not add the actor if it doesn't replicate
		if (ActorCDO || !ActorCDO->GetIsReplicated())
		{
			continue;
		}

		// Do not add SKEL and REINST classes
		FString ClassName = Class->GetName();
		if (ClassName.StartsWith("SKEL_") || ClassName.StartsWith("REINST_"))
		{
			continue;
		}

		ReplicatedClasses.Add(Class);

		// If we already have mapped it to the policy, don't do it again
		if (RepPoliciesClassMap.Contains(Class, false))
		{
			continue;
		}

		auto ShouldSpatialize = [](const AActor* Actor)
		{
			return Actor->GetIsReplicated() && (!(Actor->bAlwaysRelevant || Actor->bOnlyRelevantToOwner || Actor->bNetUseOwnerRelevancy));
		};

		UClass* SuperClass = Class->GetSuperClass();
		if (AActor* SuperCDO = Cast<AActor>(SuperClass->GetDefaultObject()))
		{
			if (SuperCDO->GetIsReplicated() == ActorCDO->GetIsReplicated()
				&& SuperCDO->bAlwaysRelevant == ActorCDO->bAlwaysRelevant
				&& SuperCDO->bOnlyRelevantToOwner == ActorCDO->bOnlyRelevantToOwner
				&& SuperCDO->bNetUseOwnerRelevancy == ActorCDO->bNetUseOwnerRelevancy)
			{
				continue;
			}

			if (ShouldSpatialize(ActorCDO) == false && ShouldSpatialize(SuperCDO) == true)
			{
				NonSpatializedClasses.Add(Class);
			}
		}

		if (ShouldSpatialize(ActorCDO) == true)
		{
			SetRule(Class, EClassRepPolicy::Spatialize_Dynamic);
		}
		else if (ActorCDO->bAlwaysRelevant && !ActorCDO->bOnlyRelevantToOwner)
		{
			SetRule(Class, EClassRepPolicy::RelevantAllConnections);
		}
	}

	// Explicitly set replication info for our classes

	TArray<UClass*> ExplicitlySetClasses;

	auto SetClassIndo = [&](UClass* Class, FClassReplicationInfo& RepInfo)
	{
		GlobalActorReplicationInfoMap.SetClassInfo(Class, RepInfo);
		ExplicitlySetClasses.Add(Class);
	};

	FClassReplicationInfo PawnClassInfo;
	PawnClassInfo.SetCullDistanceSquared(300000.f * 300000.f);
	SetClassIndo(APawn::StaticClass(), PawnClassInfo);

	for (UClass* ReplicatedClass : ReplicatedClasses)
	{
		if(ExplicitlySetClasses.FindByPredicate([&](const UClass* Class) { return ReplicatedClass->IsChildOf(Class); }) != nullptr)
		{
			continue;
		}

		bool bSpatialize = IsSpatialized(RepPoliciesClassMap.GetChecked(ReplicatedClass));

		FClassReplicationInfo ClassInfo;
		InitClassReplicationInfo(ClassInfo, ReplicatedClass, bSpatialize, NetDriver->GetNetServerMaxTickRate());
		GlobalActorReplicationInfoMap.SetClassInfo(ReplicatedClass, ClassInfo);
	}
}

void UMReplicationGraph::InitGlobalGraphNodes()
{
	/*PreAllocateRepList(3, 12);
	PreAllocateRepList(6, 12);
	PreAllocateRepList(128, 64);
	PreAllocateRepList(512, 16);*/

	// Create our grid node
	GridNode = CreateNewNode<UReplicationGraphNode_GridSpatialization2D>();
	GridNode->CellSize = GridCellSize;
	GridNode->SpatialBias = SpatialBias;

	if (bDisableSpatialRebuilding)
	{
		GridNode->AddToClassRebuildDenyList(AActor::StaticClass());
	}

	AddGlobalGraphNode(GridNode);

	// Create out always relevant node
	AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_ActorList>();
	AddGlobalGraphNode(AlwaysRelevantNode);
}

void UMReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo,
	FGlobalActorReplicationInfo& GlobalInfo)
{
	EClassRepPolicy MappingPolicy = GetMappingPolicy(ActorInfo.Class);
	switch (MappingPolicy)
	{
		case EClassRepPolicy::RelevantAllConnections:
		{
			if (ActorInfo.StreamingLevelName == NAME_None)
			{
				AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
			}
			else
			{
				FActorRepListRefView& RepList = AlwaysRelevantStreamingLevelActors.FindOrAdd(ActorInfo.StreamingLevelName);
				//RepList.PrepareForWrite(); Looks like deprecated
				RepList.ConditionalAdd(ActorInfo.Actor);
			}
			break;
		}
		case EClassRepPolicy::Spatialize_Static:
		{
			GridNode->AddActor_Static(ActorInfo, GlobalInfo);
			break;
		}
		case EClassRepPolicy::Spatialize_Dynamic:
		{
			GridNode->AddActor_Dynamic(ActorInfo, GlobalInfo);
			break;
		}
		case EClassRepPolicy::Spatialize_Dormancy:
		{
			GridNode->AddActor_Dormancy(ActorInfo, GlobalInfo);
			break;
		}
		default:
		{
		}
	}
}

void UMReplicationGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo)
{
	EClassRepPolicy MappingPolicy = GetMappingPolicy(ActorInfo.Class);
	switch (MappingPolicy)
	{
		case EClassRepPolicy::RelevantAllConnections:
		{
			if (ActorInfo.StreamingLevelName == NAME_None)
			{
				AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
			}
			else
			{
				FActorRepListRefView& RepList = AlwaysRelevantStreamingLevelActors.FindOrAdd(ActorInfo.StreamingLevelName);
				RepList.RemoveFast(ActorInfo.Actor);
			}
			break;
		}
		case EClassRepPolicy::Spatialize_Static:
		{
			GridNode->RemoveActor_Static(ActorInfo);
			break;
		}
		case EClassRepPolicy::Spatialize_Dynamic:
		{
			GridNode->RemoveActor_Dynamic(ActorInfo);
			break;
		}
		case EClassRepPolicy::Spatialize_Dormancy:
		{
			GridNode->RemoveActor_Dormancy(ActorInfo);
			break;
		}
		default:
		{
		}
	}
}

void UMReplicationGraph::InitClassReplicationInfo(FClassReplicationInfo& Info, UClass* Class, bool bSpatialize,
	float ServerMaxTickRate)
{
	if (AActor* CDO = Cast<AActor>(Class->GetDefaultObject()))
	{
		if (bSpatialize)
		{
			Info.SetCullDistanceSquared(CDO->NetCullDistanceSquared);
		}

		Info.ReplicationPeriodFrame = FMath::Max<uint32>(FMath::RoundToFloat(ServerMaxTickRate / CDO->NetUpdateFrequency), 1);
	}
}

EClassRepPolicy UMReplicationGraph::GetMappingPolicy(UClass* IN_Class)
{
	if (const auto* Policy = RepPoliciesClassMap.Get(IN_Class))
	{
		return *Policy;
	}
	return EClassRepPolicy::NotRouted;
}

//----------------------------------------------------
// UMReplicationGraphNode_AlwaysRelevant_ForConnection

void UMReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection(
	const FConnectionGatherActorListParameters& Params)
{
	Super::GatherActorListsForConnection(Params);

	auto* RepGraph = CastChecked<UMReplicationGraph>(GetOuter());

	FPerConnectionActorInfoMap& ConnectionActorInfoMap = Params.ConnectionManager.ActorInfoMap;
	TMap<FName, FActorRepListRefView>& AlwaysRelevantStreamingLevelActors = RepGraph->AlwaysRelevantStreamingLevelActors;

	for (int32 Idx = AlwaysRelevantStreamingLevels.Num() - 1; Idx >= 0; --Idx)
	{
		FName StreamingLevel = AlwaysRelevantStreamingLevels[Idx];
		FActorRepListRefView* ListPtr = AlwaysRelevantStreamingLevelActors.Find(StreamingLevel);

		if (ListPtr == nullptr)
		{
			AlwaysRelevantStreamingLevels.RemoveAtSwap(Idx, 1, false);
			continue;
		}

		FActorRepListRefView& RepList = *ListPtr;
		if (RepList.Num() > 0)
		{
			bool bAllDormant = true;
			for (FActorRepListType Actor : RepList)
			{
				FConnectionReplicationActorInfo& ConnectionActorInfo = ConnectionActorInfoMap.FindOrAdd(Actor);
				if (ConnectionActorInfo.bDormantOnConnection == false)
				{
					bAllDormant = false;
					break;
				}
			}

			if (bAllDormant)
			{
				AlwaysRelevantStreamingLevels.RemoveAtSwap(Idx, 1, false);
			}
			else
			{
				Params.OutGatheredReplicationLists.AddReplicationActorList(RepList);
			}
		}
	}
}

void UMReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityAdd(FName LevelName,
                                                                                     UWorld* LevelWorld)
{
	AlwaysRelevantStreamingLevels.Add(LevelName);
}

void UMReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityRemove(FName LevelName)
{
	AlwaysRelevantStreamingLevels.Remove(LevelName);
}

void UMReplicationGraphNode_AlwaysRelevant_ForConnection::ResetGameWorldState()
{
	AlwaysRelevantStreamingLevels.Empty();
}
