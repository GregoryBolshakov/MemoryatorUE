#include "MGroundMarker.h"

#include "Managers/MWorldGenerator.h"
#include "Managers/MRoadManager.h"

void UMGroundMarker::Initialize(AMWorldGenerator* WorldGenerator, UMRoadManager* RoadManager)
{
	pWorldGenerator = WorldGenerator;
	pRoadManager = RoadManager;
}

void UMGroundMarker::Render(float DeltaSeconds) const
{
	const auto BlockSize = pWorldGenerator->GetGroundBlockSize();
	const auto ChunkSizeInBlocks = pRoadManager->GetChunkSize();
	const FVector ChunkSizeInUnits = FVector(ChunkSizeInBlocks.X, ChunkSizeInBlocks.Y, 0.f) * BlockSize;
	const auto RegionSizeInChunks = pRoadManager->GetRegionSize();
	const auto RegionSizeInUnits = FVector(RegionSizeInChunks.X, RegionSizeInChunks.Y, 0.f) * ChunkSizeInUnits;
	for (const auto AdjacentRegion : pRoadManager->AdjacentRegions)
	{
		const auto ChunkIndex = pRoadManager->GetChunkIndexByRegion(AdjacentRegion);
		const auto BlockIndex = pRoadManager->GetBlockIndexByChunk(ChunkIndex);
		const auto Location = pWorldGenerator->GetGroundBlockLocation(BlockIndex);

		// Draw region boundaries
		DrawDebugBox(
			GetWorld(),
			Location + RegionSizeInUnits / 2.f,
			RegionSizeInUnits / 2.f,
			FColor::Red,
			true,
			-1,
			0,
			10.f
		);

		// We will split all the chunks into pairs, and then we will connect some pairs and some not
		TArray<FIntPoint> ChunkIndexes;
		for (int i = ChunkIndex.X; i < ChunkIndex.X + RegionSizeInChunks.X; ++i)
		{
			for (int j = ChunkIndex.Y; j < ChunkIndex.Y + RegionSizeInChunks.Y; ++j)
			{
				// Draw each chunk boundaries
				DrawDebugBox(
					GetWorld(),
					pWorldGenerator->GetGroundBlockLocation(pRoadManager->GetBlockIndexByChunk({i, j})) + ChunkSizeInUnits / 2.f,
					ChunkSizeInUnits / 2.f,
					FColor::Blue,
					true,
					-1,
					0,
					3.f
				);
			}
		}
	}
}
