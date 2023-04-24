// Fill out your copyright notice in the Description page of Project Settings.

#include "MBlockGenerator.h"
#include "MWorldGenerator.h"
#include "MGroundBlock.h"
#include "MActor.h"

void UMBlockGenerator::Generate(const FIntPoint BlockIndex, AMWorldGenerator* pWorldGenerator, EBiome Biome)
{
	if (!IsValid(pWorldGenerator) || !GroundBlockBPClass)
	{
		check(false);
		return;
	}

	const FVector BlockSize = pWorldGenerator->GetGroundBlockSize();
	const FVector BlockCenter = pWorldGenerator->GetGroundBlockLocation(BlockIndex) + BlockSize / 2.f;

	const auto BlockOfActors = *pWorldGenerator->GetGridOfActors().Find(BlockIndex);
	// Spawn the ground block
	FActorSpawnParameters BlockSpawnParameters;
	BlockSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (auto* GroundBlock = pWorldGenerator->SpawnActor<AMGroundBlock>(GroundBlockBPClass.Get(), pWorldGenerator->GetGroundBlockLocation(BlockIndex), FRotator::ZeroRotator, BlockSpawnParameters))
	{
		GroundBlock->UpdateBiome(Biome);
		BlockOfActors->pGroundBlock = GroundBlock; // temporary stuff, will be gone when get rid of UBlockOfActors::pGroundBlock
	}

	FOnSpawnActorStarted OnSpawnActorStarted;
	OnSpawnActorStarted.AddLambda([this, Biome](AActor* Actor)
	{
		if (const auto MActor = Cast<AMActor>(Actor))
		{
			MActor->SetBiomeForRandomization(Biome);
			return;
		}
		check(false);
	});

	const auto TreesNumber = FMath::RandRange(Preset.TreesConfig.MinNumberOfInstances, Preset.TreesConfig.MaxNumberOfInstances);
	if (Preset.TreesConfig.ToSpawnClass)
	{
		for (int i = 0; i < TreesNumber; ++i)
		{
			const auto Tree = pWorldGenerator->SpawnActorInRadius<AMActor>(Preset.TreesConfig.ToSpawnClass, BlockCenter, FRotator::ZeroRotator, {}, FMath::RandRange(0.f, static_cast<float>(BlockSize.X / 2.f)), 0.f, OnSpawnActorStarted);
		}
	}
	const auto PlantsNumber = FMath::RandRange(Preset.PlantsConfig.MinNumberOfInstances, Preset.PlantsConfig.MaxNumberOfInstances);
	if (Preset.PlantsConfig.ToSpawnClass)
	{
		for (int i = 0; i < PlantsNumber; ++i)
		{
			const auto Plant = pWorldGenerator->SpawnActorInRadius<AMActor>(Preset.PlantsConfig.ToSpawnClass, BlockCenter, FRotator::ZeroRotator, {}, FMath::RandRange(0.f, static_cast<float>(BlockSize.X / 2.f)), 0.f, OnSpawnActorStarted);
		}
	}
}

