// Fill out your copyright notice in the Description page of Project Settings.

#include "MOutpostGenerator.h"

#include "Framework/MGameMode.h"
#include "Managers/SaveManager/MWorldSaveTypes.h"
#include "StationaryActors/Outposts/MGap.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"
#include "Characters/MCharacter.h" // TODO: Remove this when refactor usage of PopulateResidentsInHouse

DEFINE_LOG_CATEGORY(LogOutpostGenerator);

void AMOutpostGenerator::GenerateOnCirclePerimeter(FVector Center, float CircleRadius,
	const TArray<UMElementDataForGeneration*>& ElementsData)
{
	UWorld* World = GetWorld();
	const auto WorldGenerator = AMGameMode::GetWorldGenerator(this);

	if (!WorldGenerator || ElementsData.IsEmpty())
	{
		check(false);
		return;
	}

	const FVector TopPoint = GetPointOnCircle(Center, CircleRadius, 0.f);

	const auto BlockSize = WorldGenerator->GetGroundBlockSize();
	const auto PCGGraphVillage = WorldGenerator->GetBlockGenerator()->GetGraph("Village");
	// Here we should clean all the blocks we are about to cover
	WorldGenerator->RegenerateArea(Center, FMath::CeilToInt(CircleRadius / FMath::Min(BlockSize.X, BlockSize.Y)), PCGGraphVillage); //TODO: Increase the area somehow! for now I don't know how to calculate it

	float DistanceFromCenter = CircleRadius;

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

	// Number of each element required to be built. Is going to get elements removed, can't rely on indexes, use pointers.
	TMap<const UMElementDataForGeneration*, int> ElementsCountData;
	for (auto& Data : ElementsData)
	{
		if (const int Number = Data->GetRandomCount(); Number > 0)
		{
			ElementsCountData.Add(Data, Number);
		}
	}

	while(!ElementsCountData.IsEmpty())
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// The class of the new building is random
		TArray<const UMElementDataForGeneration*> RemainedIndexes;
		ElementsCountData.GetKeys(RemainedIndexes);
		const int32 RandomIndex = FMath::RandRange(0, RemainedIndexes.Num() - 1);
		const auto* BuildingMetadata = RemainedIndexes[RandomIndex];

		// The temporary actor to find the position for the building
		const auto TestingBuildingActor = World->SpawnActor<AMOutpostElement>(BuildingMetadata->ToSpawnClass.Get(), TopPoint, FRotator::ZeroRotator, SpawnParameters);

		if (!TestingBuildingActor)
		{
			check(false);
			return;
		}

		ShiftBuildingRandomly(TestingBuildingActor);

		// We try to find a location to fit the building
		if (TryToPlaceBuilding(*TestingBuildingActor, BuildingIndex, DistanceFromCenter))
		{
			--ElementsCountData[BuildingMetadata];
			if (ElementsCountData[BuildingMetadata] == 0)
			{
				ElementsCountData.Remove(BuildingMetadata);
			}

			if (!BuildingMetadata->ToSpawnClass->IsChildOf(AMGap::StaticClass())) // They are supposed to be deleted
			{
				WorldGenerator->EnrollActorToGrid(TestingBuildingActor);
				if (auto* OutpostHouse = Cast<AMOutpostHouse>(TestingBuildingActor))
				{
					OutpostHouse->SetOwnerOutpost(this);
					Houses.Add(FName(OutpostHouse->GetName()), OutpostHouse);
					if (const auto* HouseMetadata = Cast<UMHouseDataForGeneration>(BuildingMetadata))
					{
						PopulateResidentsInHouse(OutpostHouse, HouseMetadata);
					}
				}
			}
		}
		else
		{
			// Previous implementation was such: If cannot place an actor, then stop and don't build the rest.

			TestingBuildingActor->AActor::Destroy(); // Used plain AActor::Destroy(), and it's OK, because spawned building didn't get EnrollActorToGrid() called

			//One of possible solutions to develop generation. It hasn't been proved yet and the binary search isn't suitable for it.
			// The idea is to keep increasing the radius of generation each time we couldn't fit an actor.
			// Obviously, the order of the actors is important, because trying to place a big one will result in an increase
			// in the generation radius, although there may still be unplaced small ones that could fit.
			// But the village should have a chaotic structure, so for now this is acceptable.
			DistanceFromCenter += 500.f; // TODO: Add a parameter for this
			// TODO: Ensure this doesn't cause endless loop
		}
	}

	// Remove all spawned Gap actors
	TArray<FName> KeysToRemove;
	for (auto It = BuildingMap.CreateIterator(); It; ++It)
	{
		if (It.Value()->GetClass()->IsChildOf(AMGap::StaticClass()))
		{
			KeysToRemove.Add(It->Key);
		}
	}
	for (const auto& Key : KeysToRemove)
	{
		BuildingMap[Key]->AActor::Destroy(); // We use plain AActor::Destroy() because Gaps were spawned not as a part of the Grid System
		BuildingMap.Remove(Key);
	}
}

FMActorSaveData AMOutpostGenerator::GetSaveData() const
{
	auto MActorSD = Super::GetSaveData();
	MActorSD.ActorSaveData.MiscBool.Add("Generated", bGenerated);

	return MActorSD;
}

void AMOutpostGenerator::BeginLoadFromSD(const FMActorSaveData& MActorSD)
{
	Super::BeginLoadFromSD(MActorSD);
	bGenerated = MActorSD.ActorSaveData.MiscBool.FindChecked("Generated");
}

void AMOutpostGenerator::ShiftBuildingRandomly(const AActor* Building)
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

//эту функцию сложно менять. Она предполагала что количество домов посчитано заранее и она вычетает конкретный дом после размещения.
//Скорее всего придется посчитать количество заранее.
bool AMOutpostGenerator::TryToPlaceBuilding(AMOutpostElement& BuildingActor, int& BuildingIndex, float& DistanceFromCenter)
{
	if (const auto Location = FindLocationForBuilding(BuildingActor, BuildingIndex, DistanceFromCenter); Location.IsSet())
	{
		BuildingActor.SetActorLocation(Location.GetValue());
		BuildingMap.Add(*BuildingActor.GetName(), &BuildingActor);
		++BuildingIndex;

		return true;
	}
	return false;
}

TOptional<FVector> AMOutpostGenerator::FindLocationForBuilding(const AMOutpostElement& BuildingActor, int BuildingIndex,
	float DistanceFromCenter) const
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
				UE_LOG(LogOutpostGenerator, Log, TEXT("Couldn't fit an actor on the circle, increase the radius"));
				break;
			}
		}
	}

	return LastValidPosition;
}

void AMOutpostGenerator::PopulateResidentsInHouse(AMOutpostHouse* HouseActor,
	const UMHouseDataForGeneration* BuildingMetadata)
{
	const auto WorldGenerator = AMGameMode::GetWorldGenerator(this);
	if (!IsValid(WorldGenerator))
		return;

	// Calculate the amount of villagers to be spawned and spawn them at the entry point of the building.
	if (const auto EntryPointComponent = Cast<USceneComponent>(HouseActor->GetDefaultSubobjectByName(TEXT("EntryPoint"))))
	{
		const auto EntryPoint = EntryPointComponent->GetComponentTransform().GetLocation();

		for (const auto& [VillagerClass, ToSpawnVillagerMetadata] : BuildingMetadata->ResidentsDataMap)
		{
			const int RequiredVillagersNumber = ToSpawnVillagerMetadata->GetRandomCount();
			for (int i = 0; i < RequiredVillagersNumber; ++i)
			{
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				const auto VillagerPawn = WorldGenerator->SpawnActor<AMCharacter>(VillagerClass.Get(), EntryPoint, FRotator::ZeroRotator, SpawnParameters, true);
				if (!VillagerPawn)
				{
					check(false);
					continue;
				}

				HouseActor->MoveResidentIn(VillagerPawn);
			}
		}
	}
}
