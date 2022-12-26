// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/DataAsset.h"
#include "MWorldManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWorldManager, Log, All);

UCLASS()
class TOPDOWNTEMP_API UWorldManagerSettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings")
		TSubclassOf<class UMWorldGenerator> WorldGeneratorBlueprint;
};

/**
World subsystem that manages the main world entities
 */
UCLASS(Config = WorldManager)
class TOPDOWNTEMP_API UMWorldManager : public UWorldSubsystem
{
	GENERATED_BODY()
private:

	UMWorldManager();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void OnWorldBeginPlay();

	UPROPERTY()
	UMWorldGenerator* WorldGenerator;

	UPROPERTY(Config)
	FString SettingsDataAssetPath;

	UPROPERTY()
	UWorldManagerSettingsDataAsset* SettingsDataAsset;
};