#include "MRoadManager.h"
#include "MWorldGenerator.h"
#include "Components/SplineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StationaryActors/MRoadSplineActor.h"

void UMRoadManager::GenerateNewPieceForRoads(const TSet<FIntPoint>& BlocksOnPerimeter, AMWorldGenerator* WorldGenerator)
{
	const auto World = GetWorld();
	if (!IsValid(World)) return;
	const auto Player = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(Player)) return;

	const auto PlayerLocation = Player->GetTransform().GetLocation();
	const auto PlayerBlock = WorldGenerator->GetGroundBlockIndex(PlayerLocation);

	const auto BlocksInRadius = WorldGenerator->GetBlocksInRadius(PlayerBlock.X, PlayerBlock.Y, WorldGenerator->GetActiveZoneRadius());

	// Iterate all blocks on the perimeter to extend already existing roads, or to lay new
	for (const auto& PerimeterBlockIndex : BlocksOnPerimeter)
	{
		const auto BlockMetadata = WorldGenerator->FindOrAddBlock(PerimeterBlockIndex);
		if (BlockMetadata->RoadSpline) // (Common case) The road section has already been laid here, try to extend it towards an adjacent block
		{
			const auto* SplineComponent = BlockMetadata->RoadSpline->GetSplineComponent();
			const auto PointsNumber = SplineComponent->GetNumberOfSplinePoints();
			const auto FirstSplinePoint = SplineComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
			const auto LastSplinePoint = SplineComponent->GetLocationAtSplinePoint(PointsNumber - 1, ESplineCoordinateSpace::World);

			bool bCanBeExtended = true;
			FIntPoint BlockForExtentionIndex;
			// Determine whether the road can be extended from this block
			if (WorldGenerator->GetGroundBlockIndex(FirstSplinePoint) == PerimeterBlockIndex || // Can continue road only from the ends
				WorldGenerator->GetGroundBlockIndex(LastSplinePoint) == PerimeterBlockIndex)
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
							if (WorldGenerator->FindOrAddBlock(IteratedBlock)->RoadSpline)
							{
								bCanBeExtended = false;
							}
							else
							{
								BlockForExtentionIndex = IteratedBlock;
							}
						}
					}
				}
				if (bCanBeExtended)
				{ // Add point to the spline from the closest end. Copy RoadSpline pointer to the block metadata
					WorldGenerator->FindOrAddBlock(BlockForExtentionIndex)->RoadSpline = BlockMetadata->RoadSpline;
					const auto NewIndex = WorldGenerator->GetGroundBlockIndex(FirstSplinePoint) == PerimeterBlockIndex ? 0 : PointsNumber;
					const auto BlockSize = WorldGenerator->GetGroundBlockSize();
					auto NewPosition = WorldGenerator->GetGroundBlockLocation(BlockForExtentionIndex) + BlockSize / 2.f;
					auto test = FMath::RandRange(-0.5f, 0.5f); //temp
					NewPosition.X += FMath::RandRange(-0.5f, 0.5f) * BlockSize.X; // Random offset
					NewPosition.Y += FMath::RandRange(-0.5f, 0.5f) * BlockSize.Y; // Random offset
					BlockMetadata->RoadSpline->GetSplineComponent()->AddSplinePointAtIndex(NewPosition, NewIndex, ESplineCoordinateSpace::World, true);
				}
			}
		}
		
	}
}
