// Fill out your copyright notice in the Description page of Project Settings.


#include "MWorldManager.h"
#include "MWorldGenerator.h"

DEFINE_LOG_CATEGORY(LogWorldManager);

UMWorldManager::UMWorldManager()
{
	LoadConfig();
	UE_LOG(LogWorldManager, Log, TEXT("Booting using Input data table at: %s"), *SettingsDataAssetPath);
	static ConstructorHelpers::FObjectFinder<UDataAsset> ActionDataTableFinder(*SettingsDataAssetPath);
	if (!ensure(ActionDataTableFinder.Succeeded()) ||
		!ensure(ActionDataTableFinder.Object))
	{
		return;
	}
	
	auto* DataAsset = ActionDataTableFinder.Object;

	SettingsDataAsset = dynamic_cast<UWorldManagerSettingsDataAsset*>(DataAsset);

	WorldGenerator = NewObject<UMWorldGenerator>(this, SettingsDataAsset->WorldGeneratorBlueprint, FName(TEXT("WorldGenerator")));
}

void UMWorldManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	GetWorld()->OnWorldBeginPlay.AddUObject(this, &UMWorldManager::OnWorldBeginPlay);
}

void UMWorldManager::OnWorldBeginPlay()
{
	WorldGenerator->GenerateWorld();
}
