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

DEFINE_LOG_CATEGORY(LogVillageGenerator);

AMVillageGenerator::AMVillageGenerator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AMVillageGenerator::Generate()
{
	Super::Generate();

	GenerateOnCirclePerimeter(GetActorLocation(), HousesCircleRadius, HousesData);

	GenerateOnCirclePerimeter(GetActorLocation(), StallsCircleRadius, StallsData);
}
