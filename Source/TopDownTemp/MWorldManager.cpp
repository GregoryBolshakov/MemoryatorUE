// Fill out your copyright notice in the Description page of Project Settings.


#include "MWorldManager.h"
#include "MWorldGenerator.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogWorldManager);

UMWorldManager::UMWorldManager()
{
	LoadConfig();
	//TODO: Consider removing this code. WorldGenerator now is an AActor, not a subsystem.
	UE_LOG(LogWorldManager, Log, TEXT("Booting using Input data table at: %s"), *SettingsDataAssetPath);
	static ConstructorHelpers::FObjectFinder<UDataAsset> ActionDataTableFinder(*SettingsDataAssetPath);
	if (!ensure(ActionDataTableFinder.Succeeded()) ||
		!ensure(ActionDataTableFinder.Object))
	{
		return;
	}
	
	auto* DataAsset = ActionDataTableFinder.Object;

	SettingsDataAsset = dynamic_cast<UWorldManagerSettingsDataAsset*>(DataAsset);
}

void UMWorldManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UMWorldManager::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	WorldGenerator = Cast<AMWorldGenerator>(UGameplayStatics::GetActorOfClass(Cast<UObject>(&InWorld), AMWorldGenerator::StaticClass()));
	check(WorldGenerator);

	WorldGenerator->GenerateWorld();
}
