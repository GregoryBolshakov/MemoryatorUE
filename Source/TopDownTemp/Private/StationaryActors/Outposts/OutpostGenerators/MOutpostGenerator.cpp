// Fill out your copyright notice in the Description page of Project Settings.

#include "MOutpostGenerator.h"

#include "Framework/MGameMode.h"
#include "Managers/SaveManager/MWorldSaveTypes.h"
#include "StationaryActors/Outposts/MGap.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"
#include "Characters/MCharacter.h" // TODO: Remove this when refactor usage of PopulateResidentsInHouse
#include "Helpers/M2DRepresentationBlueprintLibrary.h"

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
	const auto PCGGraphVillage = WorldGenerator->GetBlockGenerator()->GetGraph("Village"); // TODO: make a parameter
	// Here we should clean all the blocks we are about to cover
	WorldGenerator->RegenerateArea(Center, FMath::CeilToInt(CircleRadius / FMath::Min(BlockSize.X, BlockSize.Y)), PCGGraphVillage); //TODO: Increase the area somehow! for now I don't know how to calculate it

	// The generation goes on a circle perimeter.
	// We place the elements one by one from top to bottom, starting with the left semicircle and then alternating
	//
	//        [1st]
	//   [3rd]      [2nd]
	// [5th]         [4th] ...
	//   []          []  ...
	//    []        []   ...
	//
	//
	// We do a binary searchIn to find the closest eligible position so the element does not intersect already placed.

	// Index for "even-odd" check to know which semicircle to go
	int ElementIndex = 0;

	// Number of each element required to be built. Its elements are going to be being removed.
	// Use pointers as keys to access metadata.
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

		// The class of the new element is random
		TArray<const UMElementDataForGeneration*> RemainedIndexes;
		ElementsCountData.GetKeys(RemainedIndexes);
		const int32 RandomIndex = FMath::RandRange(0, RemainedIndexes.Num() - 1);
		const auto* ElementData = RemainedIndexes[RandomIndex];

		// A temporary actor to "try on" a position for the element
		const auto TestingElementActor = World->SpawnActor<AMOutpostElement>(ElementData->ToSpawnClass.Get(), TopPoint, FRotator::ZeroRotator, SpawnParameters);
		if (!TestingElementActor)
		{
			check(false);
			return;
		}

		// We try to find a location to fit the element
		if (const auto Location = FindLocationOnCircle(*TestingElementActor, ElementIndex, Center, CircleRadius); Location.IsSet())
		{
			TestingElementActor->SetActorLocation(Location.GetValue());
			ElementsMap.Add(FName(TestingElementActor->GetName()), TestingElementActor);
			++ElementIndex;

			--ElementsCountData[ElementData];
			if (ElementsCountData[ElementData] == 0)
			{
				ElementsCountData.Remove(ElementData);
			}

			// Enroll element to the grid and do some custom post-spawn things like populating residents
			if (!ElementData->ToSpawnClass->IsChildOf(AMGap::StaticClass())) // Skip gaps since they are going to be deleted
			{
				ProcessShiftOptions(TestingElementActor, ElementData, Center);

				WorldGenerator->EnrollActorToGrid(TestingElementActor);

				if (auto* OutpostHouse = Cast<AMOutpostHouse>(TestingElementActor))
				{
					OutpostHouse->SetOwnerOutpost(this);
					Houses.Add(FName(OutpostHouse->GetName()), OutpostHouse);
					if (const auto* HouseMetadata = Cast<UMHouseDataForGeneration>(ElementData))
					{
						PopulateResidentsInHouse(OutpostHouse, HouseMetadata);
					}
				}
			}
		}
		else
		{
			TestingElementActor->AActor::Destroy(); // Used plain AActor::Destroy(), and it's OK, because spawned element didn't get EnrollActorToGrid() called

			// Previous implementation was such: If cannot place an actor, then stop and don't build the rest.

			//One of possible solutions to develop generation. It hasn't been proved yet and the binary search isn't suitable for it.
			// The idea is to keep increasing the radius of generation each time we couldn't fit an actor.
			// Obviously, the order of the actors is important, because trying to place a big one will result in an increase
			// in the generation radius, although there may still be unplaced small ones that could fit.
			// But the village should have a chaotic structure, so for now this is acceptable.
			CircleRadius += 500.f; // TODO: Add a parameter for this
			// TODO: Ensure this doesn't cause endless loop
		}
	}

	// Remove all spawned Gap actors
	TArray<FName> KeysToRemove;
	for (auto It = ElementsMap.CreateIterator(); It; ++It)
	{
		if (It.Value()->GetClass()->IsChildOf(AMGap::StaticClass()))
		{
			KeysToRemove.Add(It->Key);
		}
	}
	for (const auto& Key : KeysToRemove)
	{
		ElementsMap[Key]->AActor::Destroy(); // We use plain AActor::Destroy() because Gaps were spawned not as a part of the Grid System
		ElementsMap.Remove(Key);
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

void AMOutpostGenerator::ProcessShiftOptions(AMOutpostElement* Element, const UMElementDataForGeneration* Data, TOptional<FVector> LocalCenter)
{
	switch (Data->ShiftOptions)
	{
	case EShiftOptions::RandomRotateAndMove:
		RotateAndMoveMeshRandomly(Element);
		break;
	case EShiftOptions::RotateToLocalCenter:
	case EShiftOptions::RotateToLocalCenterSloppy:
		if (!LocalCenter.IsSet())
		{
			check(false);
			break;
		}
		RotateMeshToPoint(Element, LocalCenter.GetValue());
		if (Data->ShiftOptions == EShiftOptions::RotateToLocalCenterSloppy)
		{
			// Add a little random rotation
			Element->AddActorLocalRotation(FRotator(0.f, FMath::RandRange(-20.f, 20.f), 0.f));
		}
		break;
	default:
		break;
	}
}

void AMOutpostGenerator::RotateAndMoveMeshRandomly(const AMOutpostElement* Element)
{
	// TODO: Rename tag "BuildingMesh" to something generic
	if (const auto MeshComponent = Cast<UStaticMeshComponent>(Element->FindComponentByTag(UStaticMeshComponent::StaticClass(), "BuildingMesh")))
	{
		const auto RandomRotation = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f);
		MeshComponent->SetRelativeRotation(RandomRotation);

		const auto MeshBounds = MeshComponent->Bounds;
		FBoxSphereBounds RandomOffsetBounds;
		Element->GetActorBounds(true, RandomOffsetBounds.Origin, RandomOffsetBounds.BoxExtent, true);

		const auto MeshLowerBound = MeshBounds.Origin - MeshBounds.BoxExtent;
		const auto MeshUpperBound = MeshBounds.Origin + MeshBounds.BoxExtent;

		const auto RandomOffsetLowerBound = RandomOffsetBounds.Origin - RandomOffsetBounds.BoxExtent;
		const auto RandomOffsetUpperBound = RandomOffsetBounds.Origin + RandomOffsetBounds.BoxExtent;

		const FVector RandomOffset = FVector(
			FMath::RandRange(RandomOffsetLowerBound.X - MeshLowerBound.X, RandomOffsetUpperBound.X - MeshUpperBound.X),
			FMath::RandRange(RandomOffsetLowerBound.Y - MeshLowerBound.Y, RandomOffsetUpperBound.Y - MeshUpperBound.Y),
			0.f);
		MeshComponent->SetRelativeLocation(MeshComponent->GetRelativeLocation() + RandomOffset);
	}
}

void AMOutpostGenerator::RotateMeshToPoint(const AMOutpostElement* Element, const FVector& Point)
{
	// TODO: Rename tag "BuildingMesh" to something generic
	if (const auto MeshComponent = Cast<UStaticMeshComponent>(Element->FindComponentByTag(UStaticMeshComponent::StaticClass(), "BuildingMesh")))
	{
		const auto Location = Element->GetActorLocation();
		MeshComponent->SetRelativeRotation(UM2DRepresentationBlueprintLibrary::GetRotationTowardPoint(Location, Point));
	}
}

TOptional<FVector> AMOutpostGenerator::FindLocationOnCircle(const AMOutpostElement& TestingElementActor, int ElementIndex,
                                                            FVector Center, float CircleRadius) const
{
	constexpr int PrecisionStepsNumber = 7; // It's impossible to know when exactly to stop
	TOptional<FVector> LastValidPosition;

	// Bounds of the binary search 
	float BottomPointAngle = PI * pow(-1, ElementIndex - 1); // Decide whether we go on the left or right semicircle
	float TopPointAngle = 0.f;

	for (int SearchStep = 0; SearchStep < PrecisionStepsNumber; ++SearchStep)
	{
		const auto Mid = (BottomPointAngle + TopPointAngle) / 2.f;

		const auto Location = GetPointOnCircle(Center, CircleRadius, Mid);
		const bool bIsEncroaching = GetWorld()->EncroachingBlockingGeometry(&TestingElementActor, Location, FRotator::ZeroRotator);
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
	const UMHouseDataForGeneration* HouseData)
{
	const auto WorldGenerator = AMGameMode::GetWorldGenerator(this);
	if (!IsValid(WorldGenerator))
		return;

	// Calculate the amount of villagers to be spawned and spawn them at the entry point of the building.
	if (const auto EntryPointComponent = Cast<USceneComponent>(HouseActor->GetDefaultSubobjectByName(TEXT("EntryPoint"))))
	{
		const auto EntryPoint = EntryPointComponent->GetComponentTransform().GetLocation();

		for (const auto& [VillagerClass, ToSpawnVillagerMetadata] : HouseData->ResidentsDataMap)
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
