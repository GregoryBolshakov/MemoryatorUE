// Fill out your copyright notice in the Description page of Project Settings.

#include "MVillageGenerator.h"

#include "Controllers/MVillagerMobController.h"
#include "MWorldGenerator.h"
#include "MWorldManager.h"
#include "Math/UnrealMathUtility.h"
#include "../StationaryActors/MActor.h"

DEFINE_LOG_CATEGORY(LogVillageGenerator);

AMVillageGenerator::AMVillageGenerator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TownSquareRadius(0)
{
}

FVector GetPointOnCircle(const FVector& CircleCenter, float Radius, float Angle)
{
	return { CircleCenter.X + Radius * cos(Angle), CircleCenter.Y + Radius * sin(Angle), 0.f };
}

void AMVillageGenerator::Generate()
{
	UWorld* pWorld = GetWorld();
	AMWorldGenerator* pWorldGenerator = nullptr;
	if (pWorld)
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			pWorldGenerator = pWorldManager->GetWorldGenerator();
		}
	}
	const auto GapMetadata = ToSpawnBuildingMetadataMap.Find("Gap");

	if (!pWorldGenerator || ToSpawnBuildingMetadataMap.IsEmpty() || !ToSpawnBuildingMetadataMap.Find("MainBuilding") || !GapMetadata)
	{
		check(false);
		return;
	}

	const FVector CenterPosition = GetTransform().GetLocation();
	const FVector TopPoint = GetPointOnCircle(CenterPosition, TownSquareRadius, 0.f);

	// Here we should clean all the blocks we are about to cover
	pWorldGenerator->CleanArea(CenterPosition, TownSquareRadius); //TODO: Increase the area somehow! for now I don't know how to calculate it

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

		const auto BuildingMetadata = ToSpawnBuildingMetadataMap.Find(BuildingClassName);
		if (!BuildingMetadata)
		{
			return;
		}

		// The temporary actor to find the position for the building
		const auto TestingBuildingActor = pWorld->SpawnActor<AActor>(BuildingMetadata->ToSpawnClass.Get(), TopPoint, FRotator::ZeroRotator, SpawnParameters);

		if (!TestingBuildingActor)
		{
			check(false);
			return;
		}

		ShiftBuildingRandomly(TestingBuildingActor);

		// We try to find a location to fit the building
		if (TryToPlaceBuilding(*TestingBuildingActor, BuildingIndex, DistanceFromCenter, BuildingClassName, *BuildingMetadata))
		{
			pWorldGenerator->EnrollActorToGrid(TestingBuildingActor);
		}
		else
		{
			// Previous implementation was such: If cannot place an actor, then stop and don't build the rest.

			TestingBuildingActor->Destroy();
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
		if (const auto MActor = Cast<AMActor>(BuildingMap[Key]))
		{
			MActor->Destroy();
			BuildingMap.Remove(Key);
		}
		else
			check(false);
	}
}

bool AMVillageGenerator::TryToPlaceBuilding(AActor& BuildingActor, int& BuildingIndex, float& DistanceFromCenter, FName BuildingClassName, const FToSpawnBuildingMetadata& BuildingMetadata)
{
	if (const auto Location = FindLocationForBuilding(BuildingActor, BuildingIndex, DistanceFromCenter); Location.IsSet())
	{
		BuildingActor.SetActorLocation(Location.GetValue());
		BuildingMap.Add(*BuildingActor.GetName(), &BuildingActor);
		++BuildingIndex;
		if (const auto NumberOfInstances = RequiredNumberOfInstances.Find(BuildingClassName))
		{
			*NumberOfInstances -= 1;
			if (*NumberOfInstances == 0)
			{
				RequiredNumberOfInstances.Remove(BuildingClassName);
			}
		}

		OnBuildingPlaced(BuildingActor, BuildingMetadata);
		return true;
	}
	return false;
}

void AMVillageGenerator::OnBuildingPlaced(AActor& BuildingActor, const FToSpawnBuildingMetadata& BuildingMetadata)
{
	UWorld* pWorld = GetWorld();
	AMWorldGenerator* pWorldGenerator = nullptr;
	if (pWorld)
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			pWorldGenerator = pWorldManager->GetWorldGenerator();
		}
	}
	if (!IsValid(pWorldGenerator))
		return;

	// Calculate the amount of villagers to be spawned and spawn them at the entry point of the building.
	if (const auto EntryPointComponent = Cast<USceneComponent>(BuildingActor.GetDefaultSubobjectByName(TEXT("EntryPoint"))))
	{
		const auto EntryPoint = EntryPointComponent->GetComponentTransform().GetLocation();

		for (const auto& [VillagerClass, ToSpawnVillagerMetadata] : BuildingMetadata.ToSpawnVillagerMetadataMap)
		{
			const int RequiredVillagersNumber = FMath::RandRange(ToSpawnVillagerMetadata.MinNumberOfInstances, ToSpawnVillagerMetadata.MaxNumberOfInstances);
			for (int i = 0; i < RequiredVillagersNumber; ++i)
			{
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				const auto VillagerPawn = pWorldGenerator->SpawnActor<APawn>(VillagerClass.Get(), EntryPoint, FRotator::ZeroRotator, SpawnParameters, true);
				if (!VillagerPawn)
				{
					check(false);
					continue;
				}

				if (const auto VillagerController = Cast<AMVillagerMobController>(VillagerPawn->Controller))
				{
					VillagerController->Initialize(BuildingActor, GetActorLocation(), TownSquareRadius);
				}
			}
		}
	}
}

void AMVillageGenerator::DetermineAllBuildingsNumberOfInstances()
{
	for (auto& [Name, Metadata] : ToSpawnBuildingMetadataMap)
	{
		if (const auto NumberOfInstances = FMath::RandRange(Metadata.MinNumberOfInstances, Metadata.MaxNumberOfInstances); NumberOfInstances > 0)
		{
			RequiredNumberOfInstances.Add(Name, NumberOfInstances);
		}
	}
}

void AMVillageGenerator::ShiftBuildingRandomly(const AActor* Building) const
{
	if (const auto BuildingMeshComponents = Building->GetComponentsByTag(UStaticMeshComponent::StaticClass(), "BuildingMesh"); !BuildingMeshComponents.IsEmpty())
	{
		const auto BuildingMeshComponent = Cast<UStaticMeshComponent>(BuildingMeshComponents[0]);
		const auto RandomRotation = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f);
		BuildingMeshComponent->SetRelativeRotation(RandomRotation);

		const auto BuildingBounds = BuildingMeshComponent->Bounds;
		FBoxSphereBounds RandomOffsetBounds;
		Building->GetActorBounds(true, RandomOffsetBounds.Origin, RandomOffsetBounds.BoxExtent, true);

		const auto BuildingLowerBound = BuildingBounds.Origin - BuildingBounds.BoxExtent;
		const auto BuildingUpperBound = BuildingBounds.Origin + BuildingBounds.BoxExtent;

		const auto RandomOffsetLowerBound = RandomOffsetBounds.Origin - RandomOffsetBounds.BoxExtent;
		const auto RandomOffsetUpperBound = RandomOffsetBounds.Origin + RandomOffsetBounds.BoxExtent;

		const FVector RandomOffset = FVector(
			FMath::RandRange(RandomOffsetLowerBound.X - BuildingLowerBound.X, RandomOffsetUpperBound.X - BuildingUpperBound.X),
			FMath::RandRange(RandomOffsetLowerBound.Y - BuildingLowerBound.Y, RandomOffsetUpperBound.Y - BuildingUpperBound.Y),
			0.f);
		BuildingMeshComponent->SetRelativeLocation(BuildingMeshComponent->GetRelativeLocation() + RandomOffset);
	}
}

TOptional<FVector> AMVillageGenerator::FindLocationForBuilding(const AActor& BuildingActor, int BuildingIndex, float DistanceFromCenter) const
{
	constexpr int PrecisionStepsNumber = 7; // It's impossible to know when exactly to stop
	TOptional<FVector> LastValidPosition;
	const FVector CenterPosition = GetTransform().GetLocation();

	// Bounds of the binary search 
	float BottomPointAngle = PI * pow(-1, BuildingIndex - 1); // Decide whether we go on the left or right semicircle
	float TopPointAngle = 0.f;

	for (int SearchStep = 0; SearchStep < PrecisionStepsNumber; ++SearchStep)
	{
		const auto Mid = (BottomPointAngle + TopPointAngle) / 2.f;

		const auto Location = GetPointOnCircle(CenterPosition, DistanceFromCenter, Mid);
		const bool bIsEncroaching = GetWorld()->EncroachingBlockingGeometry(&BuildingActor, Location, FRotator::ZeroRotator);
		if (!bIsEncroaching)
		{
			BottomPointAngle = Mid;
			LastValidPosition = Location;
			if (FMath::IsNearlyEqual(BottomPointAngle, TopPointAngle) || SearchStep == PrecisionStepsNumber - 1)
			{
				break;
			}
		}
		else
		{
			TopPointAngle = Mid;
			if (FMath::IsNearlyEqual(BottomPointAngle, TopPointAngle) || SearchStep == PrecisionStepsNumber - 1)
			{
				if (LastValidPosition.IsSet())
				{
					break;
				}
				UE_LOG(LogVillageGenerator, Log, TEXT("Couldn't fit an actor on the circle, increase the radius"));
				break;
			}
		}
	}

	return LastValidPosition;
}
