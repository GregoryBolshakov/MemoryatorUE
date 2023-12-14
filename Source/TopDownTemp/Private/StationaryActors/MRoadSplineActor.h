#pragma once

#include "CoreMinimal.h"
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
	USplineComponent* GetSplineComponent() { return SplineComponent; }

protected:
	UPROPERTY(EditAnywhere)
	USplineComponent* SplineComponent;
};

