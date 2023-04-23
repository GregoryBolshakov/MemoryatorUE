// Fill out your copyright notice in the Description page of Project Settings.


#include "MWorldManager.h"
#include "MWorldGenerator.h"
#include "MAICrowdManager.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogWorldManager);

UMWorldManager::UMWorldManager()
{
}

void UMWorldManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UMWorldManager::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	WorldGenerator = Cast<AMWorldGenerator>(UGameplayStatics::GetActorOfClass(Cast<UObject>(&InWorld), AMWorldGenerator::StaticClass()));
}
