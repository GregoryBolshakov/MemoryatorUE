#include "MRoadSplineActor.h"
#include "Components/SplineComponent.h"

AMRoadSplineActor::AMRoadSplineActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SetRootComponent(SplineComponent);
}
