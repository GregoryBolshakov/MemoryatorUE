#include "MRoadManager.h"

#include "MRoadManagerSaveTypes.h"
#include "Managers/MMetadataManager.h"
#include "Managers/SaveManager/MSaveManager.h"
#include "Managers/MWorldGenerator.h"
#include "Algo/RandomShuffle.h"
#include "Components/SplineComponent.h"
#include "Framework/MGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "StationaryActors/MRoadSplineActor.h"
#include "StationaryActors/Outposts/OutpostGenerators/MOutpostGenerator.h"

void UMRoadManager::Initialize(AMWorldGenerator* IN_WorldGenerator)
{
	pWorldGenerator = IN_WorldGenerator;
	LoadedSave = Cast<URoadManagerSave>(UGameplayStatics::LoadGameFromSlot(URoadManagerSave::SlotName, 0));
	if (!LoadedSave)
	{
		LoadedSave = Cast<URoadManagerSave>(UGameplayStatics::CreateSaveGameObject(URoadManagerSave::StaticClass()));
	}

	// Create and initialize the class responsible for debug rendering of ground geometry
	GroundMarker = GetWorld()->SpawnActor<AMGroundMarker>(FVector::ZeroVector, FRotator::ZeroRotator);
	GroundMarker->Initialize(IN_WorldGenerator, this);
}

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
	if (GetRoadActor(BlockA, BlockB, RoadType))
	{
		return;
	}

	const auto BlockSize = pWorldGenerator->GetGroundBlockSize();

	// Spawn a Road Spline and populate points towards the adjacent chunk
	auto* RoadSplineActor = GetWorld()->SpawnActor<AMRoadSplineActor>(
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
	RoadSplineActor->SetRoadType(RoadType);
	RoadSplineActor->GetSplineComponent()->RemoveSplinePoint(1, true);

	int x_inc = FMath::Sign(BlockB.X - BlockA.X), y_inc = FMath::Sign(BlockB.Y - BlockA.Y);

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

	/** Set manually replicated point positions. Spline component doesn't replicate any of its properties. */
	TArray<FVector> PointsForReplication;
	const int32 PointsCount = RoadSplineActor->GetSplineComponent()->GetNumberOfSplinePoints();
	for (int32 i = 0; i < PointsCount; ++i)
	{
		FVector PointLocation = RoadSplineActor->GetSplineComponent()->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		PointsForReplication.Add(PointLocation);
	}
	RoadSplineActor->SetPointsForReplication(PointsForReplication);

	// Adds BlockB to BlockA's connections and vice versa.
	AddConnection(BlockA, BlockB, RoadSplineActor);
}

const TSet<FIntPoint> UMRoadManager::GetAdjacentRegions(const FIntPoint& ChunkIndex) const
{
	TSet<FIntPoint> Result;
	const auto CurrentRegion = GetRegionIndexByChunk(ChunkIndex);
	Result.Add(CurrentRegion);

	// When we on an edging chunk of the region, consider regions next to the chunk as adjacent
	if ((ChunkIndex.X + 1) % RegionSize.X == 0) // Right Edge
	{
		Result.Add({CurrentRegion.X + 1, CurrentRegion.Y});
		if ((ChunkIndex.Y + 1) % RegionSize.Y == 0) // Top-Right Diagonal Edge
		{
			Result.Add({CurrentRegion.X + 1, CurrentRegion.Y + 1});
		}
		if (ChunkIndex.Y % RegionSize.Y == 0) // Bottom-Right Diagonal Edge
		{
			Result.Add({CurrentRegion.X + 1, CurrentRegion.Y - 1});
		}
	}
	if (ChunkIndex.X % RegionSize.X == 0) // Left Edge
	{
		Result.Add({CurrentRegion.X - 1, CurrentRegion.Y});
		if ((ChunkIndex.Y + 1) % RegionSize.Y == 0) // Top-Left Diagonal Edge
		{
			Result.Add({CurrentRegion.X - 1, CurrentRegion.Y + 1});
		}
		if (ChunkIndex.Y % RegionSize.Y == 0) // Bottom-Left Diagonal Edge
		{
			Result.Add({CurrentRegion.X - 1, CurrentRegion.Y - 1});
		}
	}
	if ((ChunkIndex.Y + 1) % RegionSize.Y == 0) // Top Edge
	{
		Result.Add({CurrentRegion.X, CurrentRegion.Y + 1});
	}
	if (ChunkIndex.Y % RegionSize.Y == 0) // Bottom Edge
	{
		Result.Add({CurrentRegion.X, CurrentRegion.Y - 1});
	}

	return Result;
}

auto AddObserverToRegion = [](const FIntPoint& RegionIndex)
{

};
void UMRoadManager::AddObserverToZone(const FIntPoint& ChunkIndex, const uint8 ObserverIndex)
{
	for (const auto& RegionIndex : GetAdjacentRegions(ChunkIndex))
	{
		AddObserverToRegion(RegionIndex, ObserverIndex);
	}
}

void UMRoadManager::AddObserverToRegion(const FIntPoint& RegionIndex, const uint8 ObserverIndex)
{
	AdjacentRegions.Add(RegionIndex);
	if (!GridOfRegions.Contains(RegionIndex)) // Region doesn't exist yet, load/generate it for this observer
	{
		auto& Region = GridOfRegions.Add(RegionIndex);
		Region.ObserverFlags.SetBit(ObserverIndex);
		LoadOrGenerateRegion(RegionIndex);
	}
	else // Region already existed, add this observer to the region
	{
		auto* Region = GridOfRegions.Find(RegionIndex);
		Region->ObserverFlags.SetBit(ObserverIndex);
	}
}

void UMRoadManager::MoveObserverToZone(const FIntPoint& PreviousChunk, const FIntPoint& NewChunk, const uint8 ObserverIndex)
{
	const auto NewZone = GetAdjacentRegions(NewChunk);
	const auto OldZone = GetAdjacentRegions(PreviousChunk);

	// Remove observer flag on the abandoned regions
	for (const auto RegionIndex : OldZone.Difference(NewZone))
	{
		auto* RegionMetadata = GridOfRegions.Find(RegionIndex);
		check(RegionMetadata->ObserverFlags.CheckBit(ObserverIndex));
		RegionMetadata->ObserverFlags.ClearBit(ObserverIndex);

		if (RegionMetadata->ObserverFlags.IsEmpty()) // The last observer stops observing the block
		{
			AdjacentRegions.Remove(RegionIndex);
			// TODO: Unload the region from RAM
		}
	}
	// Set observer flag on the entered regions
	for (const auto RegionIndex : NewZone.Difference(OldZone))
	{
		AddObserverToRegion(RegionIndex,ObserverIndex);
	}
}

void UMRoadManager::SaveToMemory()
{
	// Save chunks
	for (const auto& [Index, ChunkMetadata] : GridOfChunks)
	{
		FChunkSaveData ChunkSD;
		ChunkSD.bConnectedOrIgnored = ChunkMetadata.bConnectedOrIgnored;
		// Save outpost
		if (ChunkMetadata.OutpostGenerator)
		{
			auto* ActorMetadata = AMGameMode::GetMetadataManager(this)->Find(FName(ChunkMetadata.OutpostGenerator->GetName()));
			if (!ActorMetadata || !IsUidValid(ActorMetadata->Uid))
			{
				check(false);
				continue;
			}
			ChunkSD.OutpostUid = ActorMetadata->Uid;
		}
		LoadedSave->SavedChunks.FindOrAdd(Index) = ChunkSD;
	}
	// Save regions
	for (const auto& [Index, RegionMetadata] : GridOfRegions)
	{
		if (RegionMetadata.bProcessed) // We only save already PROCESSED regions!
		{
			FRegionSaveData RegionSD;
			RegionSD.Initialize(RegionMetadata);
			LoadedSave->SavedRegions.FindOrAdd(Index) = RegionSD;
		}
	}

	UGameplayStatics::SaveGameToSlot(LoadedSave, URoadManagerSave::SlotName, 0);
}

AMOutpostGenerator* UMRoadManager::SpawnOutpostGeneratorForDebugging(const FIntPoint& Chunk, TSubclassOf<AMOutpostGenerator> Class)
{
	auto& ChunkMetadata = GridOfChunks.FindOrAdd(Chunk);
	if (ChunkMetadata.OutpostGenerator)
	{
		if (ChunkMetadata.OutpostGenerator->IsGenerated())
		{
			check(false); // An outpost has already been generated here, cannot spawn here
			return nullptr;
		}
	}
	SpawnOutpostGenerator(Chunk, Class);
	// Don't affect bConnectedOrIgnored on purpose as we don't mind connecting this block with any other
	return ChunkMetadata.OutpostGenerator;
}

void UMRoadManager::AddConnection(const FIntPoint& BlockA, const FIntPoint& BlockB, AMRoadSplineActor* RoadActor)
{
	if (!RoadActor)
	{
		check(false);
		return;
	}
	auto& RegionA = GridOfRegions.FindOrAdd(GetRegionIndexByChunk(GetChunkIndexByBlock(BlockA))); // Region metadata
	auto& RegionB = GridOfRegions.FindOrAdd(GetRegionIndexByChunk(GetChunkIndexByBlock(BlockB)));
	// Regions A and B might be the same, and it is totally OK

	auto& MatrixA = RegionA.MatrixWrappers.FindOrAdd(RoadActor->GetRoadType()).Matrix; // Connectivity matrix for this road type
	auto& MatrixB = RegionB.MatrixWrappers.FindOrAdd(RoadActor->GetRoadType()).Matrix;

	auto& pConnectedRoadsA = MatrixA.FindOrAdd(BlockA); // Map of connected road actors to this block
	auto& pConnectedRoadsB = MatrixB.FindOrAdd(BlockB);

	bool AContains = pConnectedRoadsA.Map.Contains(BlockB); // Is BlockB among those connected to BlockA
	bool BContains = pConnectedRoadsB.Map.Contains(BlockA); // Is BlockA among those connected to BlockB
	check(!AContains && !BContains); // Normally the connection is either two-way or completely absent, but connecting already connected is prohibited
	// If triggered => Missing one side of the connection or it's already existed

	pConnectedRoadsA.Map.Add(BlockB, RoadActor);
	pConnectedRoadsB.Map.Add(BlockA, RoadActor);
}

const AMRoadSplineActor* UMRoadManager::GetRoadActor(const FIntPoint& BlockA, const FIntPoint& BlockB, ERoadType RoadType)
{
	auto& RegionA = GridOfRegions.FindOrAdd(GetRegionIndexByChunk(GetChunkIndexByBlock(BlockA))); // Region metadata
	auto& RegionB = GridOfRegions.FindOrAdd(GetRegionIndexByChunk(GetChunkIndexByBlock(BlockB)));

	auto& MatrixA = RegionA.MatrixWrappers.FindOrAdd(RoadType).Matrix; // Connectivity matrix for this road type
	auto& MatrixB = RegionB.MatrixWrappers.FindOrAdd(RoadType).Matrix;

	auto& pConnectedRoadsA = MatrixA.FindOrAdd(BlockA); // Map of connected road actors to this block
	auto& pConnectedRoadsB = MatrixB.FindOrAdd(BlockB);

	const auto* ppRoadFoundFromA = pConnectedRoadsA.Map.Find(BlockB);
	const auto* ppRoadFoundFromB = pConnectedRoadsB.Map.Find(BlockA);

	if (ppRoadFoundFromA && ppRoadFoundFromB)
	{
		return *ppRoadFoundFromA;
	}
	check(!ppRoadFoundFromA && !ppRoadFoundFromB);
	return nullptr;
}

FRoadActorMapWrapper UMRoadManager::GetConnections(const FIntPoint& Block, ERoadType RoadType)
{
	//TODO: Implement
	return {};
}

void UMRoadManager::TriggerOutpostGenerationForAdjacentChunks(const FIntPoint& CurrentChunk)
{
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			const FIntPoint ChunkToProcess = {CurrentChunk.X + x, CurrentChunk.Y + y};
			if (const auto ChunkMetadata = GridOfChunks.Find(ChunkToProcess); ChunkMetadata && ChunkMetadata->OutpostGenerator && !ChunkMetadata->OutpostGenerator->IsGenerated())
			{
				//ChunkMetadata.OutpostGenerator->Generate();
			}
		}
	}
}

void UMRoadManager::LoadOrGenerateRegion(const FIntPoint& RegionIndex)
{
	if (!LoadConnectionsBetweenChunksWithinRegion(RegionIndex))
	{
		GenerateConnectionsBetweenChunksWithinRegion(RegionIndex);
	}
}

void UMRoadManager::GenerateConnectionsBetweenChunksWithinRegion(const FIntPoint& RegionIndex)
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

bool UMRoadManager::LoadConnectionsBetweenChunksWithinRegion(const FIntPoint& RegionIndex)
{
	if (!LoadedSave)
		return false;
	const auto LoadedRegion = LoadedSave->SavedRegions.Find(RegionIndex);
	if (!LoadedRegion)
		return false;

	auto& RegionMetadata = GridOfRegions.FindOrAdd(RegionIndex);
	RegionMetadata.bProcessed = true;
	// Spawn saved roads
	for (const auto& [RoadType, SavedMatrixWrapper] : LoadedRegion->SavedMatrices)
	{
		for (const auto& [IndexA, SavedMapWrapper] : SavedMatrixWrapper.Matrix)
		{
			for (const auto& [IndexB, SavedRoadActor] : SavedMapWrapper.Map)
			{
				ConnectTwoBlocks(IndexA, IndexB, RoadType);
			}
		}
	}
	//TODO: if new fields are added to FRegionSaveData, extract them here

	// Remove region save data, because it is now in RAM
	LoadedSave->SavedRegions.Remove(RegionIndex);

	const auto BottomLeftChunk = GetChunkIndexByRegion(RegionIndex);
	// Iterate chunks within the region to spawn outposts
	for (int i = BottomLeftChunk.X; i < BottomLeftChunk.X + RegionSize.X; ++i)
	{
		for (int j = BottomLeftChunk.Y; j < BottomLeftChunk.Y + RegionSize.Y; ++j)
		{
			// Extract chunk save data
			if (const auto LoadedChunk = LoadedSave->SavedChunks.Find({i, j}))
			{
				// Found saved chunk, extract its data
				auto& Chunk = GridOfChunks.FindOrAdd({i, j});
				Chunk.bConnectedOrIgnored = LoadedChunk->bConnectedOrIgnored;
				if (IsUidValid(LoadedChunk->OutpostUid))
				{
					Chunk.OutpostGenerator = Cast<AMOutpostGenerator>(AMGameMode::GetSaveManager(this)->LoadMActorAndClearSD(LoadedChunk->OutpostUid, pWorldGenerator));
					check(Chunk.OutpostGenerator);
				}
			}
			else
			{
				check(false);
			}
		}
	}

	return true;
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

	const auto OutpostGenerator = pWorldGenerator->SpawnActor<AMOutpostGenerator>(Class, ChunkCenter, FRotator::ZeroRotator, {}, false);
	ChunkMetadata.OutpostGenerator = OutpostGenerator;
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

AMOutpostGenerator* UMRoadManager::GetOutpostGenerator(const FIntPoint& ChunkIndex)
{
	if (const auto ChunkMetadata = GridOfChunks.Find(ChunkIndex))
	{
		return ChunkMetadata->OutpostGenerator;
	}
	return nullptr;
}

void UMRoadManager::OnPlayerChangedChunk(const FIntPoint& OldChunk, const FIntPoint& NewChunk, const uint8 ObserverIndex)
{
	MoveObserverToZone(OldChunk, NewChunk, ObserverIndex);
}
