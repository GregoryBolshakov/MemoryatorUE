// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MOutpostGenerator.h"

#include "MVillageGenerator.generated.h"

class AMOutpostElement;
DECLARE_LOG_CATEGORY_EXTERN(LogVillageGenerator, Log, All);

class AMOutpostHouse;

//TODO: Consider creation of a parent class representing any composite structure
/**
 * The class responsible for spawning buildings, determining their, types, quantity and other specifics.
 */
UCLASS()
class TOPDOWNTEMP_API AMVillageGenerator : public AMOutpostGenerator
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Generate() override;

protected:
	UPROPERTY(Category=VillageSettings, EditDefaultsOnly, BlueprintReadOnly) 
	float HousesCircleRadius = 1500.f;

	UPROPERTY(Category=VillageSettings, EditDefaultsOnly, BlueprintReadOnly) 
	float StallsCircleRadius = 700.f;

	/** Array of UMHouseDataForGeneration* describing the main circle of houses */
	UPROPERTY(Category=VillageSettings, EditDefaultsOnly, BlueprintReadOnly, Instanced)
	TArray<UMElementDataForGeneration*> HousesData;

	UPROPERTY(Category=VillageSettings, EditDefaultsOnly, BlueprintReadOnly, Instanced)
	TArray<UMElementDataForGeneration*> StallsData;

private:
	TMap<FName, int> RequiredNumberOfInstances;
};
