// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MVillageGenerator.generated.h"

/** Describes all the data can be configured for one particular kind of villagers */
USTRUCT(BlueprintType)
struct FToSpawnVillagerMetadata
{
	GENERATED_BODY()

	UPROPERTY(Category = VillagerSettings, EditAnywhere, BlueprintReadWrite)
	int MinNumberOfInstances;

	UPROPERTY(Category = VillagerSettings, EditAnywhere, BlueprintReadWrite)
	int MaxNumberOfInstances;
};

/** Describes all the data can be configured for one particular kind of buildings */
USTRUCT(BlueprintType)
struct FToSpawnBuildingMetadata
{
	GENERATED_BODY()

	UPROPERTY(Category = BuildingSettings, EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ToSpawnClass;

	UPROPERTY(Category = BuildingSettings, EditAnywhere, BlueprintReadWrite, meta=(DisplayThumbnail = true))
	TMap<TSubclassOf<AActor>, FToSpawnVillagerMetadata> ToSpawnVillagerMetadataMap;

	UPROPERTY(Category = BuildingSettings, EditAnywhere, BlueprintReadWrite)
	int MinNumberOfInstances;

	UPROPERTY(Category = BuildingSettings, EditAnywhere, BlueprintReadWrite)
	int MaxNumberOfInstances;
};

//TODO: Consider creation of a parent class representing any composite structure
/**
 * The class responsible for spawning buildings, determining their, types, quantity and other specifics.
 */
UCLASS()
class TOPDOWNTEMP_API AMVillageGenerator : public AActor
{
	GENERATED_UCLASS_BODY()

public:

	void Generate();

protected:

	void DetermineAllBuildingsNumberOfInstances();

	void ShiftBuildingRandomly(const AActor* Building) const;

	bool TryToPlaceBuilding(AActor& BuildingActor, int& BuildingIndex, float& DistanceFromCenter, FName BuildingClassName, const FToSpawnBuildingMetadata& BuildingMetadata);

	void OnBuildingPlaced(AActor& BuildingActor, const FToSpawnBuildingMetadata& BuildingMetadata);

	TOptional<FVector> FindLocationForBuilding(const AActor& BuildingActor, int BuildingIndex, float DistanceFromCenter) const;

	UPROPERTY(Category=VillageSettings, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) 
	float TownSquareRadius;

	UPROPERTY(Category=VillageCettings, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", DisplayThumbnail))
	TMap<FName, FToSpawnBuildingMetadata> ToSpawnBuildingMetadataMap;

	TMap<FName, int> RequiredNumberOfInstances;

	UPROPERTY()
	TMap<FName, AActor*> BuildingMap;
};
