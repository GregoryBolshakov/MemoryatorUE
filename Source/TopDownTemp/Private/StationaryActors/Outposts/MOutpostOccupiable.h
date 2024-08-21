#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "StationaryActors/Outposts/MOutpostElement.h"
#include "MOutpostOccupiable.generated.h"

class AMCharacter;
class AMOutpostGenerator;

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="A position which one character can occupy"))
class UMOccupiablePointComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** The Character occupying the point. */
	UPROPERTY()
	AMCharacter* Occupant;
};

//~=============================================================================
/**
 * Base class for any element that can be occupied. A bench, stall, etc.\n
 * It is occupied temporarily. Is not habitable, not a housing.
 */
UCLASS(Blueprintable)
class AMOutpostOccupiable : public AMOutpostElement
{
	GENERATED_BODY()

public:
	AMCharacter* GetAnyOccupant() const;
	bool TryAddOccupant(AMCharacter* NewOccupant, FName Tag);
	bool AlreadyOccupying(const AMCharacter* Occupant, FName Tag);
	FVector GetOccupyingLocation(const AMCharacter* Occupant);

	//UFUNCTION()
	//void RemoveOccupant(AMCharacter* Occupant);

	UFUNCTION()
	void OnOccupantRemoved(AMCharacter* Occupant);

protected:
	virtual void PostInitializeComponents() override;

	/** These components should be created in the blueprint, get gathered in PostInitializeComponents(). */
	UPROPERTY()
	TArray<UMOccupiablePointComponent*> OccupiablePoints;
};
