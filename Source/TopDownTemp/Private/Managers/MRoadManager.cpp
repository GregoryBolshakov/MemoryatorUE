#include "MRoadManager.h"
#include "MWorldGenerator.h"
#include "Algo/RandomShuffle.h"
#include "Components/SplineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StationaryActors/MRoadSplineActor.h"

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

		// Find or spawn a Road Spline and populate points towards the adjacent chunk
		auto& RoadSplineActor = MainRoads.FindOrAdd({{prev_i, prev_j}, {i, j}});
		if (!RoadSplineActor)
		{
			const auto StartBlock = GetBlockIndexByChunk({prev_i, prev_j});
			const auto FinishBlock = GetBlockIndexByChunk({i, j});
			int x_inc = FMath::Sign(FinishBlock.X - StartBlock.X), y_inc = FMath::Sign(FinishBlock.Y - StartBlock.Y);

			const auto BlockSize = pWorldGenerator->GetGroundBlockSize();
			RoadSplineActor = GetWorld()->SpawnActor<AMRoadSplineActor>(
				pWorldGenerator->GetActorClassToSpawn("RoadSpline"),
				FVector((StartBlock.X + 0.5f) * BlockSize.X, (StartBlock.Y + 0.5f) * BlockSize.Y, 1.f), // + 0.5f to put in the center of the block
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

			int x = StartBlock.X, y = StartBlock.Y;
			do
			{
				x = x == FinishBlock.X ? x : x + x_inc;
				y = y == FinishBlock.Y ? y : y + y_inc;
				FVector NewPosition{(x + 0.5f) * BlockSize.X, (y + 0.5f) * BlockSize.Y, 1.f}; // + 0.5f to put in the center of the block
				if (x != FinishBlock.X || y != FinishBlock.Y)
				{
					NewPosition.X += FMath::RandRange(-CurveFactor, CurveFactor) * BlockSize.X; // Random offset
					NewPosition.Y += FMath::RandRange(-CurveFactor, CurveFactor) * BlockSize.Y; // Random offset
				}
				RoadSplineActor->GetSplineComponent()->AddSplinePoint(NewPosition, ESplineCoordinateSpace::World, true);
			} while (x != FinishBlock.X || y != FinishBlock.Y);
		}
	} while (i != ChunkB.X || j != ChunkB.Y);
}

void UMRoadManager::ConnectTwoBlocks(const FIntPoint& BlockA, const FIntPoint& BlockB, const ERoadType RoadType)
{
	if (BlockA == BlockB)
	{
		check(false);
		return;
	}

	const auto BlockSize = pWorldGenerator->GetGroundBlockSize();

	// Find or spawn a Road Spline and populate points towards the adjacent chunk
	auto& RoadSplineActor = Trails.FindOrAdd({BlockA, BlockB});
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
		if (!ChunkA.bProcessed || !ChunkB.bProcessed)
		{
			if (FMath::RandRange(0.f, 1.f) < ConnectionChance)
			{
				ConnectTwoChunks(ChunkIndexes[i], ChunkIndexes[i+1]);
			}
		}
		ChunkA.bProcessed = true;
		ChunkB.bProcessed = true;
	}
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
