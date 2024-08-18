// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MOutpostGenerator.h"

#include "MBanditCampGenerator.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBanditCampGenerator, Log, All);

class AMOutpostElement;
class AMOutpostHouse;

/**
 * The class responsible for spawning buildings, determining their, types, quantity and other specifics of a bandit camp.
 */
UCLASS()
class TOPDOWNTEMP_API AMBanditCampGenerator : public AMOutpostGenerator
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Generate() override;

// Data for generation
protected:
	UPROPERTY(Category=BanditCampSettings, EditDefaultsOnly, BlueprintReadOnly) 
	float CartsCircleRadius = 1000.f;

	/** Carts are bandit houses. Array of UMHouseDataForGeneration* describing the main circle of carts */
	UPROPERTY(Category=BanditCampSettings, EditDefaultsOnly, BlueprintReadOnly, Instanced)
	TArray<UMElementDataForGeneration*> CartsData;

	/** An element standing in the center of the camp */
	UPROPERTY(Category=BanditCampSettings, EditDefaultsOnly, BlueprintReadOnly, Instanced)
	UMElementDataForGeneration* CenterElement = nullptr;
};
