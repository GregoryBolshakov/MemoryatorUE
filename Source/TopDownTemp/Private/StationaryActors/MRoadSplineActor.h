#pragma once

#include "CoreMinimal.h"
#include "Managers/RoadManager/MRoadManagerTypes.h"
#include "MRoadSplineActor.generated.h"

class USplineComponent;
/**
 * Class to mark a road. Uses USplineComponent. Has no visual representation.
 */
UCLASS(Blueprintable)
class AMRoadSplineActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	ERoadType GetRoadType() const { return RoadType; }

	/** Adds a corresponding tag that PCG will use to determine the road type.\n
	 * Should be called only once! */ //TODO: Make it possible to call multiple times
	void SetRoadType(const ERoadType IN_RoadType);

	USplineComponent* GetSplineComponent() const { return SplineComponent; }

protected:
	/** The tag PCG uses to differ the road types. */
	FName GetRoadPCGTag(ERoadType IN_RoadType) const;

	UPROPERTY(EditAnywhere)
	USplineComponent* SplineComponent;

	ERoadType RoadType;
};

