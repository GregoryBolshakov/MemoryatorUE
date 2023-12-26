#include "MRoadManager.h"
#include "MWorldGenerator.h"
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

	bool toggle = true; // This flag will help in alternating between increments of i and j

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
		auto RoadSplineActor = Roads.FindOrAdd({{prev_i, prev_j}, {i, j}});
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

void UMRoadManager::GenerateNewPieceForRoads(const TSet<FIntPoint>& BlocksOnPerimeter)
{
	const auto World = GetWorld();
	if (!IsValid(World)) return;
	const auto Player = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(Player)) return;

	const auto PlayerLocation = Player->GetTransform().GetLocation();
	const auto PlayerBlock = pWorldGenerator->GetGroundBlockIndex(PlayerLocation);

	const auto BlocksInRadius = pWorldGenerator->GetBlocksInRadius(PlayerBlock.X, PlayerBlock.Y, pWorldGenerator->GetActiveZoneRadius());

	// Iterate all blocks on the perimeter to extend already existing roads, or to lay new
	for (const auto& PerimeterBlockIndex : BlocksOnPerimeter)
	{
		const auto BlockMetadata = pWorldGenerator->FindOrAddBlock(PerimeterBlockIndex);
		if (BlockMetadata->RoadSpline) // (Common case) The road section has already been laid here, try to extend it towards an adjacent block
		{
			const auto* SplineComponent = BlockMetadata->RoadSpline->GetSplineComponent();
			const auto PointsNumber = SplineComponent->GetNumberOfSplinePoints();
			const auto FirstSplinePoint = SplineComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
			const auto LastSplinePoint = SplineComponent->GetLocationAtSplinePoint(PointsNumber - 1, ESplineCoordinateSpace::World);

			bool bCanBeExtended = true;
			FIntPoint BlockForExtensionIndex{};
			// Determine whether the road can be extended from this block
			if (pWorldGenerator->GetGroundBlockIndex(FirstSplinePoint) == PerimeterBlockIndex || // Can continue road only from the ends
				pWorldGenerator->GetGroundBlockIndex(LastSplinePoint) == PerimeterBlockIndex)
			{
				for (int XOffset = -1; XOffset <= 1; ++XOffset) // Iterate all neighbours (including diagonals)
				{
					for (int YOffset = -1; YOffset <= 1; ++YOffset)
					{
						if (XOffset == 0 && YOffset == 0) // Skip the block itself
							continue;
						const auto IteratedBlock = FIntPoint(PerimeterBlockIndex.X + XOffset, PerimeterBlockIndex.Y + YOffset);
						if (!BlocksInRadius.Contains(IteratedBlock) && !BlocksOnPerimeter.Contains(IteratedBlock)) // Skip already generated blocks
						{
							if (pWorldGenerator->FindOrAddBlock(IteratedBlock)->RoadSpline)
							{
								bCanBeExtended = false;
							}
							else
							{
								BlockForExtensionIndex = IteratedBlock;
							}
						}
					}
				}
				if (bCanBeExtended)
				{ // Add point to the spline from the closest end. Copy RoadSpline pointer to the block metadata
					pWorldGenerator->FindOrAddBlock(BlockForExtensionIndex)->RoadSpline = BlockMetadata->RoadSpline;
					const auto NewIndex = pWorldGenerator->GetGroundBlockIndex(FirstSplinePoint) == PerimeterBlockIndex ? 0 : PointsNumber;
					const auto BlockSize = pWorldGenerator->GetGroundBlockSize();
					auto NewPosition = pWorldGenerator->GetGroundBlockLocation(BlockForExtensionIndex) + BlockSize / 2.f;
					NewPosition.X += FMath::RandRange(-CurveFactor, CurveFactor) * BlockSize.X; // Random offset
					NewPosition.Y += FMath::RandRange(-CurveFactor, CurveFactor) * BlockSize.Y; // Random offset
					BlockMetadata->RoadSpline->GetSplineComponent()->AddSplinePointAtIndex(NewPosition, NewIndex, ESplineCoordinateSpace::World, true);
				}
			}
		}
		
	}
}

FIntPoint UMRoadManager::GetChunkIndexByLocation(const FVector& Location) const
{
	// Intentionally cast divisible to float to result in floating number. Then floor it down.
	const auto BlockIndex = pWorldGenerator->GetGroundBlockIndex(Location);
	return FIntPoint(FMath::FloorToInt(static_cast<float>(BlockIndex.X) / ChunkSize.X),
					FMath::FloorToInt(static_cast<float>(BlockIndex.Y) / ChunkSize.Y));
}
