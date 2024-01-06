// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "MBlockGenerator.generated.h"

struct FPCGVariables;
class AMWorldGenerator;
class AMGroundBlock;
class AMActor;
class UPCGGraph;
class UBlockMetadata;
enum class EBiome : uint8;

/** Describes all the data can be configured for a one kind of objects in this block (trees/flowers/mushrooms/etc.) */
USTRUCT(BlueprintType)
struct FObjectConfig
{
	GENERATED_BODY()

	UPROPERTY(Category = ObjectConfig, EditDefaultsOnly, BlueprintReadOnly)
	int MinNumberOfInstances;

	UPROPERTY(Category = ObjectConfig, EditDefaultsOnly, BlueprintReadOnly)
	int MaxNumberOfInstances;
};

/** Describes a combination of FObjectConfigs for all kind of objects. */
USTRUCT(BlueprintType)
struct FPreset
{
	GENERATED_BODY()

	/** How often a specific block will occur among generated blocks. The higher - the less frequency */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ContentConfig)
	int Rarity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ContentConfig)
	TSet<EBiome> SupportedBiomes;

	/** The name must be either from AMWorldGenerator::ToSpawnActorClasses or
	 * be processed by SetPCGVariablesByPreset() to fill FPCGVariables */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ContentConfig)
	TMap<FName, FObjectConfig> ObjectsConfig;
};

/**
 * The class responsible for spawning block content, determining objects types, quantity and other specifics.
 */
UCLASS(Blueprintable, BlueprintType)
class TOPDOWNTEMP_API UMBlockGenerator : public UObject
{
	GENERATED_BODY()

public:

	void SpawnActorsRandomly(const FIntPoint BlockIndex, AMWorldGenerator* pWorldGenerator, UBlockMetadata* BlockMetadata, const FName& PresetName = {});

	void SpawnActorsSpecifically(const FIntPoint BlockIndex, AMWorldGenerator* pWorldGenerator, const FPCGVariables& PCGVariables);

	/** Calculate values for PCG based on the Preset */
	void SetPCGVariablesByPreset(AMGroundBlock* BlockActor, const FName PresetName, EBiome Biome);

	UPCGGraph* GetDefaultGraph();

	UPCGGraph* GetGraph(FName Name);

protected:

	/** Returns a randomly selected preset basing on their Rarity value */
	FPreset GetRandomPreset(EBiome Biome);

	//TODO: Support multiple presets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ContentConfig)
	TMap<FName, FPreset> PresetMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=ContentConfig)
	TMap<FName, UPCGGraph*> PCGGraphs;

	UPROPERTY(Category = ContentConfig, EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AMGroundBlock> GroundBlockBPClass;
};
