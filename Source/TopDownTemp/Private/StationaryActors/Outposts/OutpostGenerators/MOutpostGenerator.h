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

	static void ShiftElementRandomly(const AActor* Element); // TODO: Refactor old function

	TOptional<FVector> FindLocationOnCircle(const AMOutpostElement& TestingElementActor, int ElementIndex, FVector Center, float CircleRadius) const;

	void PopulateResidentsInHouse(AMOutpostHouse* HouseActor, const UMHouseDataForGeneration* HouseData);

	bool bGenerated = false;

	/** Outpost bounds */ // TODO: This variable is a bit vague, need some strict requirements
	UPROPERTY(EditDefaultsOnly)
	float Radius = 1750.f;

	//TODO: Consider tracking other outpost elements
	TMap<FName, AMOutpostHouse*> Houses;

	/** All elements of the outpost. */
	UPROPERTY()
	TMap<FName, AMOutpostElement*> ElementsMap;
};
