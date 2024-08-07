// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "StationaryActors/MActor.h"

#include "MOutpostGenerator.generated.h"

class AMOutpostElement;
class AMOutpostHouse;

DECLARE_LOG_CATEGORY_EXTERN(LogOutpostGenerator, Log, All);

/** Stores the Min-Max range for the number of entities */
UCLASS(BlueprintType, EditInlineNew)
class UMCountData : public UObject
{
	GENERATED_BODY()

public:
	int GetRandomCount() const { return FMath::RandRange(MinNumberOfInstances, MaxNumberOfInstances); }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int MinNumberOfInstances;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int MaxNumberOfInstances;
};

/** Describes all the data can be configured for one particular kind of resident */
UCLASS(BlueprintType, EditInlineNew)
class UMResidentDataForGeneration : public UMCountData
{
	GENERATED_BODY()
};

/** Describes all the data can be configured for one particular kind of outpost element */
UCLASS(BlueprintType, EditInlineNew)
class UMElementDataForGeneration : public UMCountData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AActor> ToSpawnClass;
};

/** Describes all the data can be configured for one particular kind of house */
UCLASS(BlueprintType, EditInlineNew)
class UMHouseDataForGeneration : public UMElementDataForGeneration
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced)
	TMap<TSubclassOf<AActor>, UMResidentDataForGeneration*> ResidentsDataMap;
};

/**
 * The base class for all outpost generators. They are responsible for spawning buildings,
 * determining their, types, quantity and other specifics within one outpost.
 */
UCLASS()
class TOPDOWNTEMP_API AMOutpostGenerator : public AMActor
{
	GENERATED_BODY()

public:
	virtual void Generate() { bGenerated = true; };

	bool IsGenerated() const { return bGenerated; }

	float GetRadius() const { return Radius; }

	static FVector GetPointOnCircle(const FVector& CircleCenter, float Radius, float Angle)
	{
		return { CircleCenter.X + Radius * cos(Angle), CircleCenter.Y + Radius * sin(Angle), 0.f };
	}

protected:
	void GenerateOnCirclePerimeter(FVector Center, float CircleRadius, const TArray<UMElementDataForGeneration*>& ElementsData);

	FMActorSaveData GetSaveData() const override;

	void BeginLoadFromSD(const FMActorSaveData& MActorSD) override;

	static void ShiftBuildingRandomly(const AActor* Building); // TODO: Refactor old function

	// TODO: Refactor old function
	bool TryToPlaceBuilding(AMOutpostElement& BuildingActor, int& BuildingIndex, float& DistanceFromCenter);

	TOptional<FVector> FindLocationForBuilding(const AMOutpostElement& BuildingActor, int BuildingIndex, float DistanceFromCenter) const;

	void PopulateResidentsInHouse(AMOutpostHouse* HouseActor, const UMHouseDataForGeneration* BuildingMetadata);

	bool bGenerated = false;

	UPROPERTY(EditDefaultsOnly)
	float Radius = 1500.f;

	//TODO: Consider tracking other outpost elements
	TMap<FName, AMOutpostHouse*> Houses;

	UPROPERTY()
	TMap<FName, AMOutpostElement*> BuildingMap;
};
