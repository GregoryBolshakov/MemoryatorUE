// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "MWorldGeneratorTypes.h"

#include "MBlockGenerator.generated.h"

class AMWorldGenerator;
class AMGroundBlock;
class AMActor;

/** Describes all the data can be configured for a one kind of objects in this block (trees/flowers/mushrooms/etc.) */
USTRUCT(BlueprintType)
struct FObjectConfig
{
	GENERATED_BODY()

	UPROPERTY(Category = ObjectConfig, EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AActor> ToSpawnClass;

	UPROPERTY(Category = ObjectConfig, EditDefaultsOnly, BlueprintReadOnly)
	int MinNumberOfInstances;

	UPROPERTY(Category = ObjectConfig, EditDefaultsOnly, BlueprintReadOnly)
	int MaxNumberOfInstances;
};

/** Describes a combination of FObjectConfigs for all kind of objects */
USTRUCT(BlueprintType)
struct FPreset
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ContentConfig, meta=(AllowPrivateAccess=true, DisplayThumbnail=true))
	FObjectConfig TreesConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ContentConfig, meta=(AllowPrivateAccess=true, DisplayThumbnail=true))
	FObjectConfig PlantsConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ContentConfig, meta=(AllowPrivateAccess=true, DisplayThumbnail=true))
	FObjectConfig RocksConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ContentConfig, meta=(AllowPrivateAccess=true, DisplayThumbnail=true))
	FObjectConfig StumpsConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ContentConfig, meta=(AllowPrivateAccess=true, DisplayThumbnail=true))
	FObjectConfig BushesConfig;
};

/**
 * The class responsible for spawning block content, determining objects types, quantity and other specifics.
 */
UCLASS(Blueprintable, BlueprintType)
class TOPDOWNTEMP_API UMBlockGenerator : public UObject
{
	GENERATED_BODY()

public:

	void Generate(const FIntPoint BlockIndex, AMWorldGenerator* pWorldGenerator, EBiome Biome);

protected:

	//TODO: Support multiple presets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ContentConfig)
	FPreset Preset;

	UPROPERTY(Category = ContentConfig, EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AMGroundBlock> GroundBlockBPClass;
};
