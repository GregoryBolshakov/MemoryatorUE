// Fill out your copyright notice in the Description page of Project Settings.

#include "MBlockGenerator.h"

#include "MMetadataManager.h"
#include "MWorldGenerator.h"
#include "PCGComponent.h"
#include "StationaryActors/MGroundBlock.h"
#include "PCGGraph.h"
#include "Framework/MGameMode.h"
#include "StationaryActors/MActor.h"

void UMBlockGenerator::SpawnActorsRandomly(const FIntPoint BlockIndex, AMWorldGenerator* pWorldGenerator, UBlockMetadata* BlockMetadata, const FName& PresetName)
{
	if (!IsValid(pWorldGenerator) || !GroundBlockBPClass)
	{
		check(false);
		return;
	}

	FActorSpawnParameters BlockSpawnParameters;
	BlockSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (auto* GroundBlock = pWorldGenerator->SpawnActor<AMGroundBlock>(GroundBlockBPClass.Get(), pWorldGenerator->GetGroundBlockLocation(BlockIndex), FRotator::ZeroRotator, BlockSpawnParameters, false))
	{
		GroundBlock->UpdateBiome(BlockMetadata->Biome);
		BlockMetadata->pGroundBlock = GroundBlock;
		if (const auto PCGComponent = Cast<UPCGComponent>(GroundBlock->GetComponentByClass(UPCGComponent::StaticClass())))
		{
			PCGComponent->SetGraph(BlockMetadata->PCGGraph);
			SetPCGVariablesByPreset(GroundBlock, PresetName, BlockMetadata->Biome);
			PCGComponent->Generate();
		}
	}
}

//TODO: Try to get rid of this and have only one but flexible SpawnActors() function
void UMBlockGenerator::SpawnActorsSpecifically(const FIntPoint BlockIndex, AMWorldGenerator* pWorldGenerator,
	const FPCGVariables& PCGVariables)
{
	//TODO: Catch up with the SpawnActorsRandomly. haven't update for a long time
	if (!IsValid(pWorldGenerator) || !GroundBlockBPClass)
	{
		check(false);
		return;
	}

	FActorSpawnParameters BlockSpawnParameters;
	BlockSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FOnSpawnActorStarted OnSpawnActorStarted;
	OnSpawnActorStarted.AddLambda([this, PCGVariables](AActor* Actor)
	{
		if (const auto BlockActor = Cast<AMGroundBlock>(Actor))
		{
			BlockActor->PCGVariables = PCGVariables;
		}
	});

	if (auto* GroundBlock = pWorldGenerator->SpawnActor<AMGroundBlock>(GroundBlockBPClass.Get(), pWorldGenerator->GetGroundBlockLocation(BlockIndex), FRotator::ZeroRotator, BlockSpawnParameters, false, OnSpawnActorStarted))
	{
		GroundBlock->UpdateBiome(PCGVariables.Biome);
		const auto BlockMetadata = AMGameMode::GetMetadataManager(this)->FindOrAddBlock(BlockIndex);
		BlockMetadata->pGroundBlock = GroundBlock;
	}
}

void UMBlockGenerator::SetPCGVariablesByPreset(AMGroundBlock* BlockActor, const FName PresetName, EBiome Biome)
{
	if (BlockActor) 
	{
		BlockActor->PCGVariables.Biome = Biome;

		FPreset Preset;
		const auto pPreset = PresetMap.Find(PresetName);
		Preset = pPreset ? *pPreset : GetRandomPreset(Biome);

		for (auto& [Name, Config] : Preset.ObjectsConfig)
		{
			const auto ObjectsNumber = FMath::RandRange(Config.MinNumberOfInstances, Config.MaxNumberOfInstances);

			if (Name == FName("Tree"))
			{
				BlockActor->PCGVariables.TreesCount = ObjectsNumber;
				continue;
			}
			if (Name == FName("Bush"))
			{
				BlockActor->PCGVariables.BushesCount = ObjectsNumber;
				continue;
			}
			if (Name == FName("Stone"))
			{
				BlockActor->PCGVariables.StonesCount = ObjectsNumber;
				continue;
			}
		}
		return;
	}
	check(false);
}

UPCGGraph* UMBlockGenerator::GetDefaultGraph()
{
	const auto DefaultGraph = PCGGraphs.FindOrAdd("Default");
	check(DefaultGraph);
	return DefaultGraph;
}

UPCGGraph* UMBlockGenerator::GetGraph(FName Name)
{
	const auto DefaultGraph = PCGGraphs.FindOrAdd(Name);
	check(DefaultGraph);
	return DefaultGraph;
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
