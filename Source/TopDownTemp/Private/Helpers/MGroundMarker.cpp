#include "MGroundMarker.h"

#include "Net/UnrealNetwork.h"
#include "Managers/MWorldGenerator.h"
#include "Managers/RoadManager/MRoadManager.h"

void AMGroundMarker::Initialize(AMWorldGenerator* WorldGenerator, UMRoadManager* RoadManager)
{
	pWorldGenerator = WorldGenerator;
	pRoadManager = RoadManager;
}

void AMGroundMarker::SetReplicatedData()
{
	ReplicatedData.Enabled = EnabledOnServer;
	if (EnabledOnServer) // Don't send the rest of the diff when disabled
	{
		ReplicatedData.AdjacentRegions.Empty();
		ReplicatedData.AdjacentRegions.Append(pRoadManager->AdjacentRegions.Array());
		ReplicatedData.BlockSize = pWorldGenerator->GetGroundBlockSize();
		ReplicatedData.ChunkSizeInBlocks = pRoadManager->GetChunkSize();
		ReplicatedData.RegionSizeInChunks = pRoadManager->GetRegionSize();
	}
}

void AMGroundMarker::RenderLocally()
{
	FlushPersistentDebugLines(GetWorld());

	if (!ReplicatedData.Enabled)
		return;

	const FVector ChunkSizeInUnits = FVector(ReplicatedData.ChunkSizeInBlocks.X, ReplicatedData.ChunkSizeInBlocks.Y, 0.f) * ReplicatedData.BlockSize;
	const auto RegionSizeInUnits = FVector(ReplicatedData.RegionSizeInChunks.X, ReplicatedData.RegionSizeInChunks.Y, 0.f) * ChunkSizeInUnits;
	for (const auto AdjacentRegion : ReplicatedData.AdjacentRegions)
	{
		const auto ChunkIndex = AdjacentRegion * ReplicatedData.RegionSizeInChunks;
		const auto BlockIndex = ChunkIndex * ReplicatedData.ChunkSizeInBlocks;
		const auto Location = FVector(BlockIndex) * ReplicatedData.BlockSize;

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

		for (int i = ChunkIndex.X; i < ChunkIndex.X + ReplicatedData.RegionSizeInChunks.X; ++i)
		{
			for (int j = ChunkIndex.Y; j < ChunkIndex.Y + ReplicatedData.RegionSizeInChunks.Y; ++j)
			{
				const auto BlockIndexByChunk = FIntPoint(i, j) * ReplicatedData.ChunkSizeInBlocks;
				// Draw each chunk boundaries
				DrawDebugBox(
					GetWorld(),
					FVector(BlockIndexByChunk) * ReplicatedData.BlockSize + ChunkSizeInUnits / 2.f,
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

void AMGroundMarker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMGroundMarker, ReplicatedData);
}

void AMGroundMarker::OnToggleDebuggingGeometry_Implementation()
{
	EnabledOnServer = !EnabledOnServer;
}
