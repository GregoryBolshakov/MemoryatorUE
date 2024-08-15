// Fill out your copyright notice in the Description page of Project Settings.

#include "MVillageGenerator.h"

#include "Controllers/MVillagerMobController.h"
#include "Managers/MBlockGenerator.h"
#include "Managers/RoadManager/MRoadManager.h"
#include "Math/UnrealMathUtility.h"
#include "StationaryActors/MActor.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"
#include "Characters/MCharacter.h"
#include "Components/SplineComponent.h"
#include "Framework/MGameMode.h"
#include "StationaryActors/MRoadSplineActor.h"

DEFINE_LOG_CATEGORY(LogVillageGenerator);

AMVillageGenerator::AMVillageGenerator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

FTimerHandle tempTimer2; //temp
void AMVillageGenerator::Generate()
{
	Super::Generate();

	auto* RoadManager = AMGameMode::GetRoadManager(this);

	// The road circle in the center of the village. Stalls are going to be around it.
	// StallsCircleRadius * 0.xf because we want the road to be an inner circle for the stalls
	StallsRoadSpline = RoadManager->CreateCircleRoadSpline(GetActorLocation(), StallsCircleRadius * 0.75f, 12, ERoadType::Trail);

	GenerateOnCirclePerimeter(GetActorLocation(), HousesCircleRadius, HousesData);

	// Create roads from each house entrance to the closest point on the stalls' road circle
	for (const auto& [Name, House] : Houses)
	{
		const FVector EntryPoint = House->GetEntryPoint();

		const auto* SplineComponent = StallsRoadSpline->GetSplineComponent();
		// Get the input key (a float representing the distance along the spline) corresponding to the closest point
		float ClosestInputKey = SplineComponent->FindInputKeyClosestToWorldLocation(EntryPoint);

		// Use the input key to get the location on the spline
		FVector ClosestPoint = SplineComponent->GetLocationAtSplineInputKey(ClosestInputKey, ESplineCoordinateSpace::World);
		
		RoadManager->CreateRoadSpline(EntryPoint, ClosestPoint, 3, ERoadType::Trail);
	}

	GetWorld()->GetTimerManager().SetTimer(tempTimer2, [this]()
	{
		GenerateOnCirclePerimeter(GetActorLocation(), StallsCircleRadius, StallsData, 50.f);
	}, 0.3f, false);
}
