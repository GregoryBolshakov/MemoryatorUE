// Fill out your copyright notice in the Description page of Project Settings.

#include "MOutpostGenerator.h"

#include "Framework/MGameMode.h"
#include "Managers/SaveManager/MWorldSaveTypes.h"
#include "StationaryActors/Outposts/MGap.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"
#include "Characters/MCharacter.h" // TODO: Remove this when refactor usage of PopulateResidentsInHouse
#include "Helpers/M2DRepresentationBlueprintLibrary.h"
#include "StationaryActors/Outposts/MOutpostStall.h"

DEFINE_LOG_CATEGORY(LogOutpostGenerator);

void AMOutpostGenerator::GenerateOnCirclePerimeter(FVector Center, float CircleRadius,
	const TArray<UMElementDataForGeneration*>& ElementsData, float RadiusIncrementStep)
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
	// TODO: Add an option to keep existing actors. E.g. stalls circle may overlap houses circle but we don't want them to be cleared.
	// TODO: VERY IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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

		// A dummy actor to "try on" a position for the element. If position fits, start treating it as a full-fledged one
		const auto DummyActor = World->SpawnActor<AMOutpostElement>(ElementData->ToSpawnClass.Get(), TopPoint, FRotator::ZeroRotator, SpawnParameters);
		if (!DummyActor)
		{
			check(false);
			return;
		}

		// We try to find a location to fit the element
		if (const auto Location = FindLocationOnCircle(*DummyActor, ElementIndex, Center, CircleRadius); Location.IsSet())
		{
			DummyActor->SetActorLocation(Location.GetValue());
			AMGameMode::GetWorldGenerator(this)->EnrollActorToGrid(DummyActor);
			++ElementIndex;

			--ElementsCountData[ElementData];
			if (ElementsCountData[ElementData] == 0)
			{
				ElementsCountData.Remove(ElementData);
			}

			PostSpawnOutpostElement(DummyActor, ElementData, Center);
		}
		else
		{
			DummyActor->AActor::Destroy(); // Used plain AActor::Destroy(), and it's OK, because spawned element didn't get EnrollActorToGrid() called

			// Previous implementation was such: If cannot place an actor, then stop and don't build the rest.

			//One of possible solutions to develop generation. It hasn't been proved yet and the binary search isn't suitable for it.
			// The idea is to keep increasing the radius of generation each time we couldn't fit an actor.
			// Obviously, the order of the actors is important, because trying to place a big one will result in an increase
			// in the generation radius, although there may still be unplaced small ones that could fit.
			// But the village should have a chaotic structure, so for now this is acceptable.
			CircleRadius += RadiusIncrementStep; // TODO: Add a parameter for this
			// TODO: Ensure this doesn't cause endless loop
		}
	}

	// Remove all spawned Gap actors
	for (int i = Elements.Num() - 1; i >= 0 ; --i)
	{
		if (Elements[i]->GetClass()->IsChildOf(AMGap::StaticClass()))
		{
			Elements[i]->AActor::Destroy(); // We use plain AActor::Destroy() because Gaps were spawned not as a part of the Grid System
			Elements.RemoveAt(i);
		}
	}
}

void AMOutpostGenerator::SpawnOutpostElementAtLocation(const UMElementDataForGeneration* Data, const FVector& Location)
{
	auto* WorldGenerator = AMGameMode::GetWorldGenerator(this);
	auto* Element = WorldGenerator->SpawnActor<AMOutpostElement>(Data->ToSpawnClass, Location, FRotator::ZeroRotator);
	PostSpawnOutpostElement(Element, Data);
}

void AMOutpostGenerator::PostSpawnOutpostElement(AMOutpostElement* OutpostElement, const UMElementDataForGeneration* Data, const FVector& LocalCenter)
{
	// Do custom post-spawn things like shift processing, populating residents, etc.

	Elements.Add(OutpostElement);

	if (!OutpostElement->GetClass()->IsChildOf(AMGap::StaticClass())) // Skip gaps since they are going to be deleted
	{
		ProcessShiftOptions(OutpostElement, Data, LocalCenter);

		OutpostElement->SetOwnerOutpost(this);
		if (auto* OutpostHouse = Cast<AMOutpostHouse>(OutpostElement))
		{
			Houses.Add(FName(OutpostHouse->GetName()), OutpostHouse);
			if (const auto* HouseMetadata = Cast<UMHouseDataForGeneration>(Data))
			{
				PopulateResidentsInHouse(OutpostHouse, HouseMetadata);
			}
		} else
		if (auto* OutpostStall = Cast<AMOutpostStall>(OutpostElement))
		{
			Stalls.Add(FName(OutpostStall->GetName()), OutpostStall);
		}
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
		Element->RotateAndMoveContentRandomly();
		break;
	case EShiftOptions::RotateToLocalCenter:
	case EShiftOptions::RotateToLocalCenterSloppy:
		if (!LocalCenter.IsSet())
		{
			check(false);
			break;
		}
		Element->RotateContentToPoint(LocalCenter.GetValue());
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
