#include "MRoadSplineActor.h"
#include "Components/SplineComponent.h"

AMRoadSplineActor::AMRoadSplineActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SetRootComponent(SplineComponent);
}

void AMRoadSplineActor::SetRoadType(const ERoadType IN_RoadType)
{
	SplineComponent->ComponentTags.Add(GetRoadPCGTag(IN_RoadType));
	RoadType = IN_RoadType;
}

FName AMRoadSplineActor::GetRoadPCGTag(ERoadType IN_RoadType) const
{
	switch (IN_RoadType)
	{
	case ERoadType::MainRoad:
		return "PCG_MainRoad";
	case ERoadType::Trail:
		return "PCG_Trail";
	default: return "";
	}
}
