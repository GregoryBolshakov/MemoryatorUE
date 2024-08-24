#pragma once

#include "CoreMinimal.h"
#include "StationaryActors/MActor.h"
#include "MOutpostElement.generated.h"

class UBoxComponent;
class AMOutpostGenerator;

//~=============================================================================
/**
 * Base class for any actor which is part of an outpost.\n It can be some building, scattered equipment, auxiliary stuff
 */
UCLASS(Blueprintable)
class AMOutpostElement : public AMActor
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable)
	const AMOutpostGenerator* GetOwnerOutpost() const { return OwnerOutpost; }

	void SetOwnerOutpost(AMOutpostGenerator* Outpost) { OwnerOutpost = Outpost; }

	void RotateContentToPoint(const FVector& Point) const;

	void RotateContentRandomly() const;
	void RotateAndMoveContentRandomly() const;

protected:

	virtual FMActorSaveData GetSaveData() const override;

	virtual void BeginLoadFromSD(const FMActorSaveData& MActorSD) override;

	UPROPERTY()
	AMOutpostGenerator* OwnerOutpost;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* ScopeForShifting;
};
