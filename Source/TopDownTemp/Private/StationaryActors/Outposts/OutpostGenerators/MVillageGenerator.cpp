// Fill out your copyright notice in the Description page of Project Settings.

#include "MVillageGenerator.h"

#include "Controllers/MVillagerMobController.h"
#include "Managers/MBlockGenerator.h"
#include "Managers/MWorldGenerator.h"
#include "Math/UnrealMathUtility.h"
#include "StationaryActors/MActor.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"
#include "Characters/MCharacter.h"
#include "Framework/MGameMode.h"
#include "StationaryActors/MRoadSplineActor.h"

DEFINE_LOG_CATEGORY(LogVillageGenerator);

AMVillageGenerator::AMVillageGenerator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AMVillageGenerator::Generate()
{
	Super::Generate();

	const auto* WorldGenerator = AMGameMode::GetWorldGenerator(this);

	// The road circle in the center of the village. Stalls are going to be around it.
	auto* StallsRoadSpline = GetWorld()->SpawnActor<AMRoadSplineActor>(
		WorldGenerator->GetActorClassToSpawn("RoadSpline"),
		GetActorLocation(),
		FRotator::ZeroRotator,
		{}
	);
	// StallsCircleRadius * 0.65f because we want the road to be an inner circle for the stalls
	UMRoadManager::AddPointsOnCircleToSpline(StallsCircleRadius * 0.65f, 12, StallsRoadSpline);
	StallsRoadSpline->SetRoadType(ERoadType::Trail);

	GenerateOnCirclePerimeter(GetActorLocation(), HousesCircleRadius, HousesData);
	GenerateOnCirclePerimeter(GetActorLocation(), StallsCircleRadius, StallsData);
}
