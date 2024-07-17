#pragma once

#include "CoreMinimal.h"
#include "StationaryActors/Outposts/MOutpostElement.h"
#include "MOutpostHouse.generated.h"

class AMCharacter;
class AMOutpostGenerator;

//~=============================================================================
/**
 * Base class for housing that is part of an outpost.\n It can be any building which accommodates several residents
 */
UCLASS(Blueprintable)
class AMOutpostHouse : public AMOutpostElement
{
	GENERATED_BODY()

public:
	/** Validates all current residents, O(n) complexity */
	bool MoveResidentIn(AMCharacter* NewResident);

	void MoveResidentOut(AMCharacter* OldResident);

	UFUNCTION(BlueprintCallable)
	FVector GetEntryPoint() const;

protected:

	UPROPERTY()
	TMap<FName, AMCharacter*> Residents;

	UPROPERTY(EditDefaultsOnly)
	int Capacity = 2;
};
