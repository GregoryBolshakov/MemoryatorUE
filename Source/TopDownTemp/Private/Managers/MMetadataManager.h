#pragma once

#include "CoreMinimal.h"
#include "MWorldGeneratorTypes.h"
#include "MMetadataManager.generated.h"

class PCGGraph;

UCLASS()
class UMMetadataManager : public UObject
{
	GENERATED_BODY()
public:

	void Initialize(UPCGGraph* IN_DefaultPCGGraph);

	void Add(FName Name, AActor* Actor, const FMUid& Uid, const FIntPoint& GroundBlockIndex);

	UActorWorldMetadata* Find(FName Name);

	UActorWorldMetadata* Find(const FMUid& Uid);

	void Remove(FName Name);

	void MoveToBlock(FName Name, const FIntPoint& NewIndex);

	UBlockMetadata*& FindOrAddBlock(const FIntPoint& Index);
	
	UBlockMetadata* FindBlock(const FIntPoint& Index);

	//TODO: add RemoveBlock when needed

	/** True if the actor is dynamic, False if static (stationary, not in terms of engine) */
	static bool IsDynamic(const AActor* Actor);

	const TMap<FIntPoint, UBlockMetadata*>* GetGrid() const { return &GridOfActors; }

private:
	/** Matches actor names with their metadata.
	* Once a world is loaded, ActorsMetadata is not immediately available. It loads in parallel.\n
	* Owns metadata, i.e. should be the only container storing by value */
	UPROPERTY()
	TMap<FName, UActorWorldMetadata*> ActorsMetadata;

	/** Matches unique identifiers with actor metadata. Covers only those existing in the current game session (not all that are saved). */
	UPROPERTY()
	TMap<FMUid, UActorWorldMetadata*> UidToMetadata;

	UPROPERTY()
	TMap<FIntPoint, UBlockMetadata*> GridOfActors;

	//Here might be other mappings. Should store metadata by reference

private: // Misc //TODO: Reconsider how to handle this
	UPROPERTY()
	UPCGGraph* DefaultPCGGraph;
};
