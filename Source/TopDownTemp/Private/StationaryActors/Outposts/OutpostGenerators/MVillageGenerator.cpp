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

/*void AMVillageGenerator::Generate()
{
	Super::Generate(); // Mark as generated before any error can occur

	UWorld* World = GetWorld();
	const auto WorldGenerator = AMGameMode::GetWorldGenerator(this);

	const auto GapMetadata = HousesData.Find("Gap");

	if (!WorldGenerator || HousesData.IsEmpty() || !HousesData.Find("Palace") || !GapMetadata)
	{
		check(false);
		return;
	}

	const FVector CenterPosition = GetTransform().GetLocation();
	const FVector TopPoint = GetPointOnCircle(CenterPosition, TownSquareRadius, 0.f);

	const auto BlockSize = WorldGenerator->GetGroundBlockSize();
	const auto PCGGraphVillage = WorldGenerator->GetBlockGenerator()->GetGraph("Village");
	// Here we should clean all the blocks we are about to cover
	WorldGenerator->RegenerateArea(CenterPosition, FMath::CeilToInt(TownSquareRadius / FMath::Min(BlockSize.X, BlockSize.Y)), PCGGraphVillage); //TODO: Increase the area somehow! for now I don't know how to calculate it

	float DistanceFromCenter = TownSquareRadius;

	// The Village has a circle shape with a hollow at the bottom.
	// The Main Building is at the top point.
	// We randomly place other buildings along the left and right semicircles
	//
	//        [MAIN]
	//     []        [] Other buildings
	//   []           [] ...
	//   []           [] ...
	//    []        []   ...
	//
	//
	//  In order to find the closest eligible position where the building does not intersect with already placed we
	//  perform a binary search. The start (left) position is the bottom point of the circle or the position corresponding to the angle PI

	// Generate buildings in the given amount
	int BuildingIndex = 0;
	DetermineAllBuildingsNumberOfInstances();
	while(!RequiredNumberOfInstances.IsEmpty())
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// The class of the new building is random
		TArray<FName> KeysArray;
		RequiredNumberOfInstances.GetKeys(KeysArray);
		const int32 RandomIndex = FMath::RandRange(0, KeysArray.Num() - 1);
		const auto BuildingClassName = KeysArray[RandomIndex];

		const auto BuildingMetadata = HousesData.Find(BuildingClassName);
		if (!BuildingMetadata)
		{
			return;
		}

		// The temporary actor to find the position for the building
		const auto TestingBuildingActor = World->SpawnActor<AActor>(BuildingMetadata->ToSpawnClass.Get(), TopPoint, FRotator::ZeroRotator, SpawnParameters);

		if (!TestingBuildingActor)
		{
			check(false);
			return;
		}

		ShiftBuildingRandomly(TestingBuildingActor);

		// We try to find a location to fit the building
		if (TryToPlaceBuilding(*TestingBuildingActor, BuildingIndex, DistanceFromCenter, BuildingClassName, *BuildingMetadata))
		{
			if (BuildingMetadata->ToSpawnClass != GapMetadata->ToSpawnClass) // They are supposed to be deleted
			{
				WorldGenerator->EnrollActorToGrid(TestingBuildingActor);
				if (const auto OutpostHouse = Cast<AMOutpostHouse>(TestingBuildingActor))
				{
					OutpostHouse->SetOwnerOutpost(this);
					Houses.Add(FName(OutpostHouse->GetName()), OutpostHouse);
					PopulateResidentsInHouse(OutpostHouse, BuildingMetadata);
				}
			}
		}
		else
		{
			// Previous implementation was such: If cannot place an actor, then stop and don't build the rest.

			TestingBuildingActor->Destroy(); // Used plain AActor::Destroy(), and it's OK, because spawned building didn't get EnrollActorToGrid() called

			//One of possible solutions to develop generation. It hasn't been proved yet and the binary search isn't suitable for it.
			// The idea is to keep increasing the radius of generation each time we couldn't fit an actor.
			// Obviously, the order of the actors is important, because trying to place a big one will result in an increase
			// in the generation radius, although there may still be unplaced small ones that could fit.
			// But the village should have a chaotic structure, so for now this is acceptable.
			DistanceFromCenter += AMWorldGenerator::GetDefaultBounds(GapMetadata->ToSpawnClass, GetWorld()).SphereRadius;
			// TODO: Ensure this doesn't cause endless loop
		}
	}

	// Remove all spawned Gap actors
	TArray<FName> KeysToRemove;
	for (auto It = BuildingMap.CreateIterator(); It; ++It)
	{
		if (It.Value()->GetClass() == GapMetadata->ToSpawnClass)
		{
			KeysToRemove.Add(It->Key);
		}
	}
	for (const auto& Key : KeysToRemove)
	{
		BuildingMap[Key]->Destroy(); // We use plain AActor::Destroy() because Gaps were spawned not as a part of the Grid System
		BuildingMap.Remove(Key);
	}
}*/

void AMVillageGenerator::Generate()
{
	Super::Generate();

	GenerateOnCirclePerimeter(GetActorLocation(), Radius, HousesData);
}
