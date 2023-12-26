// Fill out your copyright notice in the Description page of Project Settings.

#include "MBlockGenerator.h"
#include "MWorldGenerator.h"
#include "StationaryActors/MGroundBlock.h"
#include "StationaryActors/MActor.h"

void UMBlockGenerator::SpawnActors(const FIntPoint BlockIndex, AMWorldGenerator* pWorldGenerator, EBiome Biome, const FName& PresetName)
{
	if (!IsValid(pWorldGenerator) || !GroundBlockBPClass)
	{
		check(false);
		return;
	}

	const FVector BlockSize = pWorldGenerator->GetGroundBlockSize();
	const FVector BlockCenter = pWorldGenerator->GetGroundBlockLocation(BlockIndex) + BlockSize / 2.f;

	const auto BlockMetadata = pWorldGenerator->FindOrAddBlock(BlockIndex);
	// Spawn the ground block
	FActorSpawnParameters BlockSpawnParameters;
	BlockSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FOnSpawnActorStarted OnSpawnActorStarted;
	OnSpawnActorStarted.AddLambda([this, PresetName, Biome](AActor* Actor)
	{
		SetVariablesByPreset(Actor, PresetName, Biome);
	});

	if (auto* GroundBlock = pWorldGenerator->SpawnActor<AMGroundBlock>(GroundBlockBPClass.Get(), pWorldGenerator->GetGroundBlockLocation(BlockIndex), FRotator::ZeroRotator, BlockSpawnParameters, false, OnSpawnActorStarted))
	{
		GroundBlock->UpdateBiome(Biome);
		BlockMetadata->pGroundBlock = GroundBlock; // temporary stuff, will be gone when get rid of UBlockMetadata::pGroundBlock
	}
}

void UMBlockGenerator::SetVariablesByPreset(AActor* Actor, const FName PresetName, EBiome Biome)
{
	if (const auto GroundBlock = Cast<AMGroundBlock>(Actor))
	{
		GroundBlock->PCG_Biome = Biome;

		FPreset Preset;
		const auto pPreset = PresetMap.Find(PresetName);
		Preset = pPreset ? *pPreset : GetRandomPreset(Biome);

		for (auto& [Name, Config] : Preset.ObjectsConfig)
		{
			const auto ObjectsNumber = FMath::RandRange(Config.MinNumberOfInstances, Config.MaxNumberOfInstances);

			if (Name == FName("Tree"))
			{
				GroundBlock->PCG_TreesCount = ObjectsNumber;
				continue;
			}
			if (Name == FName("Bush"))
			{
				GroundBlock->PCG_BushesCount = ObjectsNumber;
				continue;
			}
			if (Name == FName("Stone"))
			{
				GroundBlock->PCG_StonesCount = ObjectsNumber;
				continue;
			}
		}
		return;
	}
	check(false);
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

FPreset UMBlockGenerator::GetRandomPreset(EBiome Biome)
{
	const auto RandNumber = FMath::RandRange(0.f, 1.f);

	TArray<FPreset> SufficientRarityPresets;
	for (const auto& [Name, Preset] : PresetMap)
	{
		if (Preset.SupportedBiomes.Contains(Biome) && 1.f / Preset.Rarity >= RandNumber)
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
