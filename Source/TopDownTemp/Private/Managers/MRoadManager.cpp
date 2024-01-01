#include "MRoadManager.h"
#include "MWorldGenerator.h"
#include "Algo/RandomShuffle.h"
#include "Components/SplineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StationaryActors/MRoadSplineActor.h"

void UMRoadManager::ConnectTwoChunks(const FIntPoint& ChunkA, const FIntPoint& ChunkB)
{
	if (ChunkA == ChunkB)
	{
		check(false);
		return;
	}

	const auto BlockSize = pWorldGenerator->GetGroundBlockSize();

	int i = ChunkA.X, j = ChunkA.Y, i_inc = FMath::Sign(ChunkB.X - ChunkA.X), j_inc = FMath::Sign(ChunkB.Y - ChunkA.Y);
	do {
		int prev_i = i, prev_j = j;

		if (i != ChunkB.X)
		{
			i += i_inc;
		}
		else
		{
			if (j != ChunkB.Y)
			{
				j += j_inc;
			}
		}

		// Find or spawn a Road Spline and populate points towards the adjacent chunk
		auto& RoadSplineActor = Roads.FindOrAdd({{prev_i, prev_j}, {i, j}});
		if (!RoadSplineActor)
		{
			const auto StartBlock = GetBlockIndexByChunk({prev_i, prev_j});
			const auto FinishBlock = GetBlockIndexByChunk({i, j});
			int x_inc = FMath::Sign(FinishBlock.X - StartBlock.X), y_inc = FMath::Sign(FinishBlock.Y - StartBlock.Y);

			RoadSplineActor = GetWorld()->SpawnActor<AMRoadSplineActor>(
				pWorldGenerator->GetActorClassToSpawn("RoadSpline"),
				FVector((StartBlock.X + 0.5f) * BlockSize.X, (StartBlock.Y + 0.5f) * BlockSize.Y, 1.f), // + 0.5f to put in the center of the block
				FRotator::ZeroRotator,
				{}
			);
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

void UMRoadManager::ConnectChunksWithinRegion(/*const FIntPoint& Center (OLD)*/ const FIntPoint& RegionIndex)
{
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

FIntPoint UMRoadManager::GetRegionIndexByChunk(const FIntPoint& ChunkIndex) const
{
	return FIntPoint(FMath::FloorToInt(static_cast<float>(ChunkIndex.X) / ChunkSize.X),
					FMath::FloorToInt(static_cast<float>(ChunkIndex.Y) / ChunkSize.Y));
}

void UMRoadManager::OnPlayerChangedChunk(const FIntPoint& OldChunk, const FIntPoint& NewChunk)
{
	// When we approach the edge of the current region, we process the neighboring region
	if (NewChunk.X % RegionSize.X == RegionSize.X - 1)
	{
		ConnectChunksWithinRegion(GetRegionIndexByChunk({NewChunk.X + 1, NewChunk.Y}));
		return;
	}
	if (NewChunk.X % RegionSize.X == 0)
	{
		ConnectChunksWithinRegion(GetRegionIndexByChunk({NewChunk.X - 1, NewChunk.Y}));
		return;
	}
	if (NewChunk.Y % RegionSize.Y == RegionSize.Y - 1)
	{
		ConnectChunksWithinRegion(GetRegionIndexByChunk({NewChunk.X, NewChunk.Y + 1}));
		return;
	}
	if (NewChunk.Y % RegionSize.Y == 0)
	{
		ConnectChunksWithinRegion(GetRegionIndexByChunk({NewChunk.X, NewChunk.Y - 1}));
		return;
	}
	// We ignore diagonal cases as we process blocks with a stepped pattern (don't cut corners)
}
