#include "MRoadManager.h"
#include "MWorldGenerator.h"
#include "Algo/RandomShuffle.h"
#include "Components/SplineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StationaryActors/MRoadSplineActor.h"
#include "StationaryActors/Outposts/OutpostGenerators/MOutpostGenerator.h"

void UMRoadManager::ConnectTwoChunks(FIntPoint ChunkA, FIntPoint ChunkB, const ERoadType RoadType)
{
	FIntPoint OriginalChunkA = ChunkA;
	FIntPoint OriginalChunkB = ChunkB;

	if (ChunkA == ChunkB)
	{
		check(false);
		return;
	}

	// Chunk origin point is bottom left by default. In order to reach other 3 edges of chunk we need to select the proper adjacent chunk
	const int SignX = FMath::Sign(ChunkB.X - ChunkA.X), SignY = FMath::Sign(ChunkB.Y - ChunkA.Y);
	if (ChunkA.X < ChunkB.X && ChunkA.Y < ChunkB.Y)
	{
		ChunkA += FIntPoint(1, 1);
	}
	else if (ChunkA.X < ChunkB.X && ChunkA.Y >= ChunkB.Y)
	{
		ChunkA += FIntPoint(1, 0);
		ChunkB +=FIntPoint(0, -SignY);
	}
	else if (ChunkA.X >= ChunkB.X && ChunkA.Y < ChunkB.Y)
	{
		ChunkA += FIntPoint(0, 1);
		ChunkB += FIntPoint(-SignX, 0);
	}
	else
	{
		Swap(ChunkA, ChunkB);
		ConnectTwoChunks(ChunkA, ChunkB, RoadType);
		return;
	}

	ConnectTwoBlocks(GetBlockIndexByChunk(ChunkA), GetChunkCenterBlock(OriginalChunkA), ERoadType::Trail);
	ConnectTwoBlocks(GetBlockIndexByChunk(ChunkB), GetChunkCenterBlock(OriginalChunkB), ERoadType::Trail);

	if (ChunkA == ChunkB)
		return;

	int i = ChunkA.X, j = ChunkA.Y;
	do {
		int prev_i = i, prev_j = j;

		if (i != ChunkB.X)
		{
			i += SignX;
		}
		else
		{
			if (j != ChunkB.Y)
			{
				j += SignY;
			}
		}

		ConnectTwoBlocks(GetBlockIndexByChunk({prev_i, prev_j}), GetBlockIndexByChunk({i, j}), ERoadType::MainRoad);
	} while (i != ChunkB.X || j != ChunkB.Y);
}

void UMRoadManager::ConnectTwoBlocks(const FIntPoint& BlockA, const FIntPoint& BlockB, const ERoadType RoadType)
{
	if (BlockA == BlockB)
	{
		check(false);
		return;
	}

	// Check if there's no other kind of road between the blocks
	for (const ERoadType OtherRoadType : TEnumRange<ERoadType>())
	{
		if (OtherRoadType != RoadType)
		{
			auto& OtherRoadContainer = GetRoadContainer(OtherRoadType);
			if (OtherRoadContainer.Contains({BlockA, BlockB}))
			{
				check(false);
				return;
			}
		}
	}

	const auto BlockSize = pWorldGenerator->GetGroundBlockSize();

	// Find or spawn a Road Spline and populate points towards the adjacent chunk
	auto& RoadContainer = GetRoadContainer(RoadType);
	auto& RoadSplineActor = RoadContainer.FindOrAdd({BlockA, BlockB});
	if (!RoadSplineActor)
	{
		int x_inc = FMath::Sign(BlockB.X - BlockA.X), y_inc = FMath::Sign(BlockB.Y - BlockA.Y);

		RoadSplineActor = GetWorld()->SpawnActor<AMRoadSplineActor>(
			pWorldGenerator->GetActorClassToSpawn("RoadSpline"),
			FVector((BlockA.X + 0.5f) * BlockSize.X, (BlockA.Y + 0.5f) * BlockSize.Y, 1.f), // + 0.5f to put in the center of the block
			FRotator::ZeroRotator,
			{}
		);
		if (!RoadSplineActor)
		{
			check(false);
			return;
		}
		RoadSplineActor->Tags.Add(GetRoadPCGTag(RoadType));
		RoadSplineActor->GetSplineComponent()->RemoveSplinePoint(1, true);

		int x = BlockA.X, y = BlockA.Y;
		do
		{
			x = x == BlockB.X ? x : x + x_inc;
			y = y == BlockB.Y ? y : y + y_inc;
			FVector NewPosition{(x + 0.5f) * BlockSize.X, (y + 0.5f) * BlockSize.Y, 1.f}; // + 0.5f to put in the center of the block
			if (x != BlockB.X || y != BlockB.Y)
			{
				NewPosition.X += FMath::RandRange(-CurveFactor, CurveFactor) * BlockSize.X; // Random offset
				NewPosition.Y += FMath::RandRange(-CurveFactor, CurveFactor) * BlockSize.Y; // Random offset
			}
			RoadSplineActor->GetSplineComponent()->AddSplinePoint(NewPosition, ESplineCoordinateSpace::World, true);
		} while (x != BlockB.X || y != BlockB.Y);
	}
}

void UMRoadManager::ProcessAdjacentChunks(const FIntPoint& CurrentChunk)
{
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			const FIntPoint ChunkToProcess = {CurrentChunk.X + x, CurrentChunk.Y + y};
			const auto& ChunkMetadata = GridOfChunks.FindOrAdd(ChunkToProcess);
			if (ChunkMetadata.OutpostGenerator && !ChunkMetadata.OutpostGenerator->IsGenerated())
			{
				//ChunkMetadata.OutpostGenerator->Generate();
			}
		}
	}
}

void UMRoadManager::ProcessAdjacentRegions(const FIntPoint& CurrentChunk)
{
	const auto CurrentRegion = GetRegionIndexByChunk(CurrentChunk);

	// Self check for reliability
	ProcessRegionIfUnprocessed({CurrentRegion.X, CurrentRegion.Y});

	// When we approach the edge of the current region, we process the neighboring region
	if ((CurrentChunk.X + 1) % RegionSize.X == 0) // Right Edge
	{
		ProcessRegionIfUnprocessed({CurrentRegion.X + 1, CurrentRegion.Y});
		if ((CurrentChunk.Y + 1) % RegionSize.Y == 0) // Top-Right Diagonal Edge
		{
			ProcessRegionIfUnprocessed({CurrentRegion.X + 1, CurrentRegion.Y + 1});
		}
		if (CurrentChunk.Y % RegionSize.Y == 0) // Bottom-Right Diagonal Edge
		{
			ProcessRegionIfUnprocessed({CurrentRegion.X + 1, CurrentRegion.Y - 1});
		}
	}
	if (CurrentChunk.X % RegionSize.X == 0) // Left Edge
	{
		ProcessRegionIfUnprocessed({CurrentRegion.X - 1, CurrentRegion.Y});
		if ((CurrentChunk.Y + 1) % RegionSize.Y == 0) // Top-Left Diagonal Edge
		{
			ProcessRegionIfUnprocessed({CurrentRegion.X - 1, CurrentRegion.Y + 1});
		}
		if (CurrentChunk.Y % RegionSize.Y == 0) // Bottom-Left Diagonal Edge
		{
			ProcessRegionIfUnprocessed({CurrentRegion.X - 1, CurrentRegion.Y - 1});
		}
	}
	if ((CurrentChunk.Y + 1) % RegionSize.Y == 0) // Top Edge
	{
		ProcessRegionIfUnprocessed({CurrentRegion.X, CurrentRegion.Y + 1});
	}
	if (CurrentChunk.Y % RegionSize.Y == 0) // Bottom Edge
	{
		ProcessRegionIfUnprocessed({CurrentRegion.X, CurrentRegion.Y - 1});
	}

	ProcessAdjacentChunks(CurrentChunk);
}

void UMRoadManager::ProcessRegionIfUnprocessed(const FIntPoint& Region)
{
	if (!GridOfRegions.FindOrAdd(Region).bProcessed)
	{
		ConnectChunksWithinRegion(Region);
	}
}

void UMRoadManager::ConnectChunksWithinRegion(const FIntPoint& RegionIndex)
{
	GridOfRegions.FindOrAdd(RegionIndex).bProcessed = true;
	const auto BottomLeftChunk = GetChunkIndexByRegion(RegionIndex);

	// We will split all the chunks into pairs, and then we will connect some pairs and some not
	TArray<FIntPoint> ChunkIndexes;
	for (int i = BottomLeftChunk.X; i < BottomLeftChunk.X + RegionSize.X; ++i)
	{
		for (int j = BottomLeftChunk.Y; j < BottomLeftChunk.Y + RegionSize.Y; ++j)
		{
			ChunkIndexes.Add({i, j});
		}
	}
	Algo::RandomShuffle(ChunkIndexes);
	// Split the array into pairs and connect some of them depending on ConnectionChance
	for (int i = 0; i < ChunkIndexes.Num() - 1; ++i)
	{
		auto& ChunkA = GridOfChunks.FindOrAdd(ChunkIndexes[i]);
		auto& ChunkB = GridOfChunks.FindOrAdd(ChunkIndexes[i+1]);
		if (!ChunkA.bConnectedOrIgnored || !ChunkB.bConnectedOrIgnored)
		{
			if (FMath::RandRange(0.f, 1.f) < ConnectionChance)
			{
				SpawnOutpostGenerator(ChunkIndexes[i]);
				SpawnOutpostGenerator(ChunkIndexes[i+1]);
				ConnectTwoChunks(ChunkIndexes[i], ChunkIndexes[i+1]);
			}
		}
		ChunkA.bConnectedOrIgnored = true;
		ChunkB.bConnectedOrIgnored = true;
	}
}

void UMRoadManager::SpawnOutpostGenerator(const FIntPoint& Chunk, TSubclassOf<AMOutpostGenerator> Class)
{
	auto& ChunkMetadata = GridOfChunks.FindOrAdd(Chunk);
	if (ChunkMetadata.OutpostGenerator)
	{
		return; // Only one outpost per chunk
	}
	if (!Class)
	{
		if (OutpostBPClasses.Num() > 0)
		{
			TArray<FName> Keys;
			OutpostBPClasses.GetKeys(Keys);
			const FName RandomKey = Keys[FMath::RandRange(0, Keys.Num() - 1)];
			Class = OutpostBPClasses[RandomKey];
		}
		else
		{
			check(false);
			return;
		}
	}

	const auto ChunkCenter = pWorldGenerator->GetGroundBlockLocation(GetChunkCenterBlock(Chunk)); //TODO: Fix that, currently it's not the precise center
	const auto OutpostGenerator = GetWorld()->SpawnActor<AMOutpostGenerator>(Class, ChunkCenter, FRotator::ZeroRotator);
	ChunkMetadata.OutpostGenerator = OutpostGenerator;
}

TMap<FUnorderedConnection, AMRoadSplineActor*>& UMRoadManager::GetRoadContainer(ERoadType RoadType)
{
	switch (RoadType)
	{
	case ERoadType::MainRoad:
		return MainRoads;
	case ERoadType::Trail:
		return Trails;
	}
	check(false);
	return MainRoads;
}

FIntPoint UMRoadManager::GetChunkIndexByLocation(const FVector& Location) const
{
	const auto BlockIndex = pWorldGenerator->GetGroundBlockIndex(Location);
	return GetChunkIndexByBlock(BlockIndex);
}

FIntPoint UMRoadManager::GetChunkIndexByBlock(const FIntPoint& BlockIndex) const
{
	return FIntPoint(FMath::FloorToInt(static_cast<float>(BlockIndex.X) / ChunkSize.X),
					FMath::FloorToInt(static_cast<float>(BlockIndex.Y) / ChunkSize.Y));
}

FIntPoint UMRoadManager::GetChunkCenterBlock(const FIntPoint& ChunkIndex) const
{
	const auto BottomLeftBlock = GetBlockIndexByChunk(ChunkIndex);
	return { BottomLeftBlock.X + FMath::CeilToInt(ChunkSize.X / 2.f) - 1, BottomLeftBlock.Y + FMath::CeilToInt(ChunkSize.Y / 2.f) - 1};
}

FIntPoint UMRoadManager::GetRegionIndexByChunk(const FIntPoint& ChunkIndex) const
{
	return FIntPoint(FMath::FloorToInt(static_cast<float>(ChunkIndex.X) / RegionSize.X),
					FMath::FloorToInt(static_cast<float>(ChunkIndex.Y) / RegionSize.Y));
}

void UMRoadManager::OnPlayerChangedChunk(const FIntPoint& OldChunk, const FIntPoint& NewChunk)
{
	ProcessAdjacentRegions(NewChunk);
}

FName UMRoadManager::GetRoadPCGTag(ERoadType RoadType) const
{
	switch (RoadType)
	{
	case ERoadType::MainRoad:
		return "PCG_MainRoad";
	case ERoadType::Trail:
		return "PCG_Trail";
	}
	return "";
}
