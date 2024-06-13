#include "MRoadSplineActor.h"

#include "Net/UnrealNetwork.h"
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

void AMRoadSplineActor::SetPointsForReplication(const TArray<FVector>& Points)
{
	check(HasAuthority());
	ReplicatedPoints = Points;
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

void AMRoadSplineActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMRoadSplineActor, ReplicatedPoints);
}

void AMRoadSplineActor::OnPointsReplicated()
{
	SplineComponent->SetSplinePoints(ReplicatedPoints, ESplineCoordinateSpace::World);
}
