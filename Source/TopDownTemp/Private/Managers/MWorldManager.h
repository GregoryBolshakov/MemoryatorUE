// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/DataAsset.h"
#include "MWorldManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWorldManager, Log, All);

/**
World subsystem that manages the main world entities
 */
UCLASS(Config = WorldManager)
class TOPDOWNTEMP_API UMWorldManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	class AMWorldGenerator* GetWorldGenerator() const { return WorldGenerator; }

private:

	UMWorldManager();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UPROPERTY()
	AMWorldGenerator* WorldGenerator;
};