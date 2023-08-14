// Fill out your copyright notice in the Description page of Project Settings.

#include "MBlockGenerator.h"
#include "MWorldGenerator.h"
#include "StationaryActors/MGroundBlock.h"
#include "StationaryActors/MActor.h"

void UMBlockGenerator::Generate(const FIntPoint BlockIndex, AMWorldGenerator* pWorldGenerator, EBiome Biome, const FName& PresetName)
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

	FPreset Preset;
	const auto pPreset = PresetMap.Find(PresetName);
	Preset = pPreset ? *pPreset : GetRandomPreset();

	for (const auto& [BPClass, Config] : Preset.ObjectsConfig)
	{
		const auto ObjectsNumber = FMath::RandRange(Config.MinNumberOfInstances, Config.MaxNumberOfInstances);
		if (BPClass)
		{
			for (int i = 0; i < ObjectsNumber; ++i)
			{
				const auto Tree = pWorldGenerator->SpawnActorInRadius<AMActor>(BPClass, BlockCenter, FRotator::ZeroRotator, {}, FMath::RandRange(0.f, static_cast<float>(BlockSize.X / 2.f)), 0.f, OnSpawnActorStarted);
			}
		}
	}
}

FPreset GetRandomPresetWithHighestRarity(const TArray<FPreset>& SufficientRarityPresets)
{
	TArray<FPreset> HighestRarityPresets;
	int32 HighestRarity = SufficientRarityPresets[0].Rarity;

	// collect all elements with highest rarity
	for (const FPreset& Preset : SufficientRarityPresets)
	{
		if (Preset.Rarity == HighestRarity)
		{
			HighestRarityPresets.Add(Preset);
		}
		else
		{
			// as array is sorted, we can break the loop as soon as we reach a different rarity
			break;
		}
	}

	// choose a random element from HighestRarityPresets
	if (HighestRarityPresets.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, HighestRarityPresets.Num() - 1);
		return HighestRarityPresets[RandomIndex];
	}

	// return default FPreset if there is no element (this should never happen)
	return FPreset();
}

FPreset UMBlockGenerator::GetRandomPreset()
{
	const auto RandNumber = FMath::RandRange(0.f, 1.f);

	TArray<FPreset> SufficientRarityPresets;
	for (const auto& [Name, Preset] : PresetMap)
	{
		if (1.f / Preset.Rarity >= RandNumber)
		{
			SufficientRarityPresets.Add(Preset);
		}
	}

	if (SufficientRarityPresets.IsEmpty())
	{
		ensure(false);
		return {};
	}

	SufficientRarityPresets.Sort([](const FPreset& A, const FPreset& B)
	{
		return A.Rarity > B.Rarity;
	});

	return GetRandomPresetWithHighestRarity(SufficientRarityPresets);
}
