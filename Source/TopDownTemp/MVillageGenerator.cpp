// Fill out your copyright notice in the Description page of Project Settings.

#include "MVillageGenerator.h"

#include "MWorldGenerator.h"
#include "MWorldManager.h"
#include "Components/BoxComponent.h"
#include "Components/ShapeComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/SCS_Node.h"

FBoxSphereBounds GetDefaultBounds(const TSubclassOf<AActor> InActorClass)
{
	FBoxSphereBounds Result;
	Result.Origin = FVector::ZeroVector;
	Result.BoxExtent = FVector::ZeroVector;
	Result.SphereRadius = 0.f;

	if (!IsValid(InActorClass))
	{
		return Result;
	}

	UClass* ActorClass = InActorClass;

	// Go down the inheritance tree to find nodes that were added to parent blueprints of our blueprint graph.
	do
	{
		if (const auto ActorBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(ActorClass))
		{
			const TArray<USCS_Node*>& ActorBlueprintNodes =
				ActorBlueprintGeneratedClass->SimpleConstructionScript->GetAllNodes();

			for (const USCS_Node* Node : ActorBlueprintNodes)
			{
				if (const auto PrimitiveComponent = Cast<UPrimitiveComponent>(Node->ComponentTemplate))
				{
					const auto ComponentBounds = PrimitiveComponent->CalcBounds(PrimitiveComponent->GetComponentTransform());
					Result.BoxExtent.X = FMath::Max(Result.BoxExtent.X, ComponentBounds.BoxExtent.X);
					Result.BoxExtent.Y = FMath::Max(Result.BoxExtent.Y, ComponentBounds.BoxExtent.Y);
					Result.BoxExtent.Z = FMath::Max(Result.BoxExtent.Z, ComponentBounds.BoxExtent.Z);
				}
			}
		}

		ActorClass = Cast<UClass>(ActorClass->GetSuperStruct());

	} while (ActorClass != AActor::StaticClass());

	return Result;
}

AMVillageGenerator::AMVillageGenerator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TownSquareRadius(0)
{
}

FVector GetPointOnCircle(const FVector& CircleCenter, float Radius, float Angle)
{
	return { CircleCenter.X + Radius * cos(Angle), CircleCenter.Y + Radius * sin(Angle), 0 };
}

void AMVillageGenerator::Generate()
{
	AMWorldGenerator* pWorldGenerator = nullptr;
	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			pWorldGenerator = pWorldManager->GetWorldGenerator();
		}
	}
	if (!pWorldGenerator || ToSpawnBuildingsMetadata.IsEmpty() || !ToSpawnBuildingsMetadata.Find("MainBuilding"))
	{
		check(false);
		return;
	}

	const FVector CenterPosition = GetTransform().GetLocation();
	const FVector TopPoint = GetPointOnCircle(CenterPosition, TownSquareRadius, 0.f);

	// Here we should clean all the blocks we are about to cover
	pWorldGenerator->CleanArea(CenterPosition, TownSquareRadius); //TODO: Increase the area somehow! for now I don't know how to calculate it

	float Radius = TownSquareRadius;

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
		FActorSpawnParameters SpawnParameters{};
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.Name = FName("Building_" + FString::FromInt(BuildingIndex));

		// The class of the new building is random
		TArray<TSubclassOf<AActor>> KeysArray;
		RequiredNumberOfInstances.GetKeys(KeysArray);
		const int32 RandomIndex = FMath::RandRange(0, KeysArray.Num() - 1);
		const auto BuildingClass = KeysArray[RandomIndex];

		const auto BuildingActor = pWorldGenerator->SpawnActor<AActor>(BuildingClass.Get(), TopPoint, FRotator::ZeroRotator, SpawnParameters);
		if (!BuildingActor)
		{
			check(false);
			return;
		}

		ShiftBuildingRandomly(BuildingActor);

		if (const auto Location = FindLocationForBuilding(BuildingActor, BuildingIndex, Radius); Location.IsSet())
		{
			BuildingActor->SetActorLocation(Location.GetValue());
			BuildingMap.Add(*BuildingActor->GetName(), BuildingActor);
			++BuildingIndex;
			if (const auto NumberOfInstances = RequiredNumberOfInstances.Find(BuildingClass))
			{
				*NumberOfInstances -= 1;
				if (*NumberOfInstances == 0)
				{
					RequiredNumberOfInstances.Remove(BuildingClass);
				}
			}
		}
		else
		{
			BuildingActor->Destroy();
			//One of possible solutions to develop generation. It hasn't been proved yet and the binary search isn't suitable for it. 
			/*if (const auto GapMetadata = ToSpawnBuildingsMetadata.Find("Gap"))
			{
				const auto BoxExtent = GetDefaultBounds(GapMetadata->ToSpawnClass).BoxExtent;
				Radius += FMath::Max3(BoxExtent.X, BoxExtent.Y, BoxExtent.Z) / 2.f;
				continue;
			}*/
			check(false);
			break;
		}
	}

	// Remove all spawned Gap actors
	if (const auto GapMetadata = ToSpawnBuildingsMetadata.Find("Gap"))
	{
		for (auto It = BuildingMap.CreateIterator(); It; ++It)
		{
			if (It.Value()->GetClass() == GapMetadata->ToSpawnClass.Get())
			{
				It.Value()->Destroy();
				It.RemoveCurrent();
			}
		}
	}
}

void AMVillageGenerator::DetermineAllBuildingsNumberOfInstances()
{
	for (auto& [Name, Metadata] : ToSpawnBuildingsMetadata)
	{
		if (const auto NumberOfInstances = FMath::RandRange(Metadata.MinNumberOfInstances, Metadata.MaxNumberOfInstances); NumberOfInstances > 0)
		{
			RequiredNumberOfInstances.Add(Metadata.ToSpawnClass, NumberOfInstances);
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

TOptional<FVector> AMVillageGenerator::FindLocationForBuilding(const AActor* Building, int BuildingIndex, float Radius) const
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

		const auto Location = GetPointOnCircle(CenterPosition, Radius, Mid);
		const bool bIsEncroaching = GetWorld()->EncroachingBlockingGeometry(Building, Location, {});
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
				//check(false);
				break;
			}
		}
	}

	return LastValidPosition;
}