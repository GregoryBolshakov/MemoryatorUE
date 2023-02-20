// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MVillageGenerator.generated.h"

USTRUCT(BlueprintType)
struct FToSpawnBuildingMetadata
{
	GENERATED_BODY()

	UPROPERTY(Category=BuildingSettings, EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ToSpawnClass;

	UPROPERTY(Category=BuildingSettings, EditAnywhere, BlueprintReadWrite)
	int MinNumberOfInstances;

	UPROPERTY(Category=BuildingSettings, EditAnywhere, BlueprintReadWrite)
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

	TOptional<FVector> FindLocationForBuilding(const AActor* Building, int BuildingIndex) const;

	UPROPERTY(Category=VillageSettings, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) 
	float Radius;

	UPROPERTY(Category=VillageCettings, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", DisplayThumbnail))
	TMap<FName, FToSpawnBuildingMetadata> ToSpawnBuildingsMetadata;

	TMap<TSubclassOf<AActor>, int> RequiredNumberOfInstances;

	UPROPERTY()
	TMap<FName, AActor*> BuildingMap;
};
