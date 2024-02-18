#pragma once

#include "MMetadataManager.h"
#include "Components/MIsActiveCheckerComponent.h"

void UMMetadataManager::Initialize(UPCGGraph* IN_DefaultPCGGraph)
{
	DefaultPCGGraph = IN_DefaultPCGGraph;
}

void UMMetadataManager::Add(FName Name, AActor* Actor, const FMUid& Uid, const FIntPoint& GroundBlockIndex)
{
	if (ActorsMetadata.Contains(Name) || UidToMetadata.Contains(Uid)) // If add new mappings, must be processed here
	{
		check(false);
		return;
	}
	auto* Metadata = NewObject<UActorWorldMetadata>();
	Metadata->Actor = Actor;
	Metadata->Uid = Uid;
	Metadata->GroundBlockIndex = GroundBlockIndex;
	ActorsMetadata.Add(Name, Metadata);
	UidToMetadata.Add(Uid, Metadata);
}

UActorWorldMetadata* UMMetadataManager::Find(FName Name)
{
	const auto ppMetadata = ActorsMetadata.Find(Name);
	return ppMetadata ? *ppMetadata : nullptr;
}

UActorWorldMetadata* UMMetadataManager::Find(const FMUid& Uid)
{
	const auto ppMetadata = UidToMetadata.Find(Uid);
	return ppMetadata ? *ppMetadata : nullptr;
}

void UMMetadataManager::Remove(FName Name)
{
	if (const auto* Metadata = Find(Name))
	{
		if (auto* BlockMetadata = FindBlock(Metadata->GroundBlockIndex))
		{
			// Remove actor metadata from block
			auto& ListToRemove = IsDynamic(Metadata->Actor) ? BlockMetadata->DynamicActors : BlockMetadata->StaticActors;
			ListToRemove.Remove(Name);

			// Process block constancy
			if (const auto ActiveCheckerComp = Cast<UMIsActiveCheckerComponent>(Metadata->Actor->GetComponentByClass(UMIsActiveCheckerComponent::StaticClass())))
			{
				if (ActiveCheckerComp->GetAlwaysEnabled())
				{
					BlockMetadata->ConstantActorsCount--;
				}
			}
		}
		else check(false);

		UidToMetadata.Remove(Metadata->Uid);
		// If add new mappings, must be processed here
		ActorsMetadata.Remove(Name);
		Metadata->Actor->AActor::Destroy();
	}
	else check(false);
}

void UMMetadataManager::MoveToBlock(FName Name, const FIntPoint& NewIndex)
{
	if (auto* Metadata = Find(Name))
	{
		auto* OldBlockMetadata = FindBlock(Metadata->GroundBlockIndex);
		auto* NewBlockMetadata = FindBlock(NewIndex);
		if (OldBlockMetadata && NewBlockMetadata)
		{
			auto& ListToRemove = IsDynamic(Metadata->Actor) ? OldBlockMetadata->DynamicActors : OldBlockMetadata->StaticActors;
			ListToRemove.Remove(Name);
			auto& ListToAdd = IsDynamic(Metadata->Actor) ? NewBlockMetadata->DynamicActors : NewBlockMetadata->StaticActors;
			ListToAdd.Add(Name, Metadata->Actor);

			// Process block constancy
			if (const auto ActiveCheckerComp = Cast<UMIsActiveCheckerComponent>(Metadata->Actor->GetComponentByClass(UMIsActiveCheckerComponent::StaticClass())))
			{
				if (ActiveCheckerComp->GetAlwaysEnabled())
				{
					OldBlockMetadata->ConstantActorsCount--;
					NewBlockMetadata->ConstantActorsCount--;
				}
			}
		}
		else check(false);

		Metadata->GroundBlockIndex = NewIndex;
	}
	else check(false);
}

UBlockMetadata*& UMMetadataManager::FindOrAddBlock(const FIntPoint& Index)
{
	auto& BlockMetadata = GridOfActors.FindOrAdd(Index);
	if (!BlockMetadata)
	{
		BlockMetadata = NewObject<UBlockMetadata>(this);

		// Set default PCG graph
		BlockMetadata->PCGGraph = DefaultPCGGraph;

		return BlockMetadata;
	}
	return BlockMetadata;
}

UBlockMetadata* UMMetadataManager::FindBlock(const FIntPoint& Index)
{
	if (auto** ppBlockMetadata = GridOfActors.Find(Index))
	{
		return *ppBlockMetadata;
	}
	return nullptr;
}

bool UMMetadataManager::IsDynamic(const AActor* Actor)
{
	return Actor->GetClass()->IsChildOf<APawn>();
}
