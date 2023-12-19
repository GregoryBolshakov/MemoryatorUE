#include "MRoadManager.h"
#include "MWorldGenerator.h"
#include "Components/SplineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StationaryActors/MRoadSplineActor.h"

void UMRoadManager::ConnectTwoChunks(const FIntPoint& ChunkA, const FIntPoint& ChunkB)
{
	const int MinX = FMath::Min(ChunkA.X, ChunkB.X);
	const int MaxX = FMath::Min(ChunkA.Y, ChunkB.Y);
	const int MinY = FMath::Min(ChunkA.X, ChunkB.X);
	const int MaxY = FMath::Max(ChunkA.Y, ChunkB.Y);

	int i = MinX, j = MinY;
	while (i != MaxX || j != MaxY)
	{
		int prev_i = i, prev_j = j;
		i = i < MaxX ? i + 1 : i;
		j = j < MaxY ? j + 1 : j;
		const auto RoadSplineActorPtr = Roads.Find({{prev_i, prev_j}, {i, j}});
		if (!RoadSplineActorPtr)
		{
			GetWorld()->SpawnActor<AMRoadSplineActor>(pWorldGenerator->GetActorClassToSpawn("RoadSpline"), FVector(600.f, 200.f, 1.f), FRotator::ZeroRotator, {});
		}
	}
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
					auto test = FMath::RandRange(-0.5f, 0.5f); //temp
					NewPosition.X += FMath::RandRange(-0.5f, 0.5f) * BlockSize.X; // Random offset
					NewPosition.Y += FMath::RandRange(-0.5f, 0.5f) * BlockSize.Y; // Random offset
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
