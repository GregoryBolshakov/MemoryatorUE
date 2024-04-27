// Fill out your copyright notice in the Description page of Project Settings.

#include "MWorldGenerator.h"

#include "Math/UnrealMathUtility.h"
#include "StationaryActors/MActor.h" 
#include "Characters/MCharacter.h"
#include "StationaryActors/MGroundBlock.h"
#include "MBlockGenerator.h"
#include "MMetadataManager.h"
#include "MCommunicationManager.h"
#include "MDropManager.h"
#include "MReputationManager.h"
#include "MExperienceManager.h"
#include "Managers/RoadManager/MRoadManager.h"
#include "Components/MIsActiveCheckerComponent.h"
#include "StationaryActors/Outposts/OutpostGenerators/MVillageGenerator.h"
#include "NavigationSystem.h"
#include "Controllers/MPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NakamaManager/Private/NakamaManager.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "StationaryActors/MPickableActor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/SplineMeshActor.h"
#include "Framework/MGameMode.h"
#include "GameFramework/PlayerState.h"
#include "SaveManager/MSaveManager.h"
#include "StationaryActors/MRoadSplineActor.h"

AMWorldGenerator::AMWorldGenerator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}

FTimerHandle tempTimer; //temp
void AMWorldGenerator::InitSurroundingArea(const FIntPoint& PlayerBlock)
{
	auto* pWorld = GetWorld();
	if (!pWorld)
		return;

	const auto RoadManager = AMGameMode::GetRoadManager(this);
	const auto PlayerChunk = RoadManager->GetChunkIndexByBlock(PlayerBlock);
	RoadManager->ProcessAdjacentRegions(PlayerChunk);

	//temp
	//RoadManager->ConnectTwoChunks(PlayerChunk, {PlayerChunk.X + 1, PlayerChunk.Y-1});

	// We add 1 to the radius on purpose. Generated area always has to be further then visible
	auto BlocksInRadius = GetBlocksInRadius(PlayerBlock.X, PlayerBlock.Y, ActiveZoneRadius + 1);

	auto* MetadataManager = AMGameMode::GetMetadataManager(this);
	// Set the biomes in a separate pass first because we need to know each biome during block generation in order to disable/enable block transitions
	for (const auto BlockInRadius : BlocksInRadius)
	{
		if (MetadataManager->FindBlock(BlockInRadius) == nullptr) // Set biome only for non existent blocks
		{
			const auto BlockMetadata = AMGameMode::GetMetadataManager(this)->FindOrAddBlock(BlockInRadius);
			BlockMetadata->Biome = BiomeForInitialGeneration;
		}
	}
	for (const auto BlockInRadius : BlocksInRadius)
	{
		LoadOrGenerateBlock(BlockInRadius, false);
	}

	/*if (!AMGameMode::GetSaveManager(this)->IsLoaded()) // spawn a village (testing purposes). Only if there was no save.
	{
		pWorld->GetTimerManager().SetTimer(tempTimer, [this, PlayerChunk]()
		{
			const auto RoadManager = AMGameMode::GetRoadManager(this);
			const auto VillageClass = RoadManager->GetOutpostBPClasses().Find("Village")->Get();
			const auto VillageGenerator = RoadManager->SpawnOutpostGeneratorForDebugging(PlayerChunk, VillageClass);
			VillageGenerator->Generate();
			UpdateNavigationMesh();
		}, 0.3f, false);
	}*/

	/*EmptyBlock({PlayerBlockIndex.X, PlayerBlockIndex.Y}, true);
	BlockGenerator->SpawnActors({PlayerBlockIndex.X, PlayerBlockIndex.Y}, this, EBiome::BirchGrove, "TestBlock");*/
}

UBlockMetadata* AMWorldGenerator::EmptyBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects)
{
	const auto pWorld = GetWorld();
	if (!pWorld)
		return nullptr;

	const auto MetadataManager = AMGameMode::GetMetadataManager(this);

	// Get the block from grid or add if doesn't exist
	auto* BlockMetadata = MetadataManager->FindOrAddBlock(BlockIndex);

	// Empty actor maps
	for (auto It = BlockMetadata->StaticActors.CreateIterator(); It; ++It)
	{
		MetadataManager->Remove(It->Key);
	}
	check(BlockMetadata->StaticActors.IsEmpty());

	if (!KeepDynamicObjects)
	{
		for (auto It = BlockMetadata->DynamicActors.CreateIterator(); It; ++It)
		{
			MetadataManager->Remove(It->Key);
		}
		check(BlockMetadata->DynamicActors.IsEmpty());
	}

	//TODO: Should consider removing block's saved data as well. To prevent actors loaded by dependant actors.
	// (e.g. there was a house on the block, player moved away, restarted the game, got to the block,
	// regenerate it but then after a while player meets a resident of the house, which triggers the load of the house.
	// As the result, the house appears on an already generated block that does not provide for it).
	//TODO: This should be done carefully and require thorough testing.
	// For now it's mostly OK because all dependant actors make block constant.

	return BlockMetadata;
}

int playerCount = 0; // Temporarily hardcode UniqueID for players for offline testing
void AMWorldGenerator::ProcessConnectingPlayer(APlayerController* NewPlayer)
{
	// Function uses UniqueID field of controller's PlayerState to either load the existing character from save or spawn a new one.

	if (!NewPlayer || !NewPlayer->PlayerState)
	{
		check(false);
		return;
	}
	auto* SaveManager = AMGameMode::GetSaveManager(this);
	//const auto UniqueID = NewPlayer->PlayerState->GetUniqueId().ToString();
	const auto UniqueID = FName("BOLSHAKOV-" + FString::FromInt(playerCount++));
	auto Uid = SaveManager->FindMUidByUniqueID(FName(UniqueID));

	APawn* pPlayer = nullptr;
	if (!IsUidValid(Uid)) // First time a player with such UniqueID is logging in. Spawn a pawn for them and generate a Uid for it.
	{
		if (const auto PlayerClass = ToSpawnActorClasses.Find("Player")) // TODO: There might be different classes for players, aka different races and whatnot
		{
			UpdateActiveZone(FIntPoint::ZeroValue);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Name = FName(UniqueID);
			pPlayer = SpawnActor<AMCharacter>(*PlayerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams, true);

			const auto* Metadata = AMGameMode::GetMetadataManager(this)->Find(UniqueID);
			SaveManager->AddMUidByUniqueID(UniqueID, Metadata->Uid); // Uid was generated when spawning new AMCharacter, map it to the UniqueID
		}
	}
	else // Load character from SaveManager using the UniqueId
	{
		UpdateActiveZone(SaveManager->GetMCharacterBlock(Uid));
		pPlayer = SaveManager->LoadMCharacterAndClearSD(Uid, this);

		// Undo the forced disabling caused by SaveManager.
		// (We are not like Rust and inactive players don't come with loaded block,
		// hence save manager always forcibly disables players loaded along with a block)
		if (const auto ActiveChecker = pPlayer->GetComponentByClass<UMIsActiveCheckerComponent>())
		{
			ActiveChecker->SetAlwaysDisabled(false);
			ActiveChecker->EnableOwner();
		}
	}

	if (NewPlayer->HasActorBegunPlay())
	{
		NewPlayer->Possess(pPlayer); // Possess just spawned player by controller
	}
	else if (auto* MPlayerController = Cast<AMPlayerController>(NewPlayer))
	{
		MPlayerController->DeferredPawnToPossess = pPlayer; // Possess the player later, after controller's BeginPlay()
	}

	InitSurroundingArea(GetGroundBlockIndex(pPlayer->GetActorLocation())); // Init surroundings before spawning player so it doesn't fall underground

	// Bind to the player-moves-to-another-block event
	AMGameMode::GetExperienceManager(this)->ExperienceAddedDelegate.AddDynamic(Cast<AMPlayerController>(NewPlayer), &AMPlayerController::OnExperienceAdded);
	if (const auto PlayerMetadata = AMGameMode::GetMetadataManager(this)->Find(FName(pPlayer->GetName())))
	{
		PlayerMetadata->OnBlockChangedDelegate.AddDynamic(this, &AMWorldGenerator::OnPlayerChangedBlock);
		PlayerMetadata->OnChunkChangedDelegate.AddDynamic(AMGameMode::GetRoadManager(this), &UMRoadManager::OnPlayerChangedChunk);
	}

	//TODO: Make sure connecting player doesn't get stuck in terrain which might appear while he is away
}

void AMWorldGenerator::LoadOrGenerateBlock(const FIntPoint& BlockIndex, bool bRegenerationFeature)
{
	auto* BlockMetadata = AMGameMode::GetMetadataManager(this)->FindOrAddBlock(BlockIndex);
	if (IsValid(BlockMetadata->pGroundBlock)) // The block already exists in the current session
	{
		if (BlockMetadata->ConstantActorsCount > 0 || !bRegenerationFeature) // No need to regenerate existing block
		{
			return;
		}
		// Re-generate the block
		EmptyBlock(BlockIndex, true);
		BlockGenerator->SpawnActorsRandomly(BlockIndex, this, BlockMetadata);
	}
	else // The block doesn't exist in the current session. BUT THERE MIGHT BE DYNAMIC AND/OR CONSTANT ACTORS
	{
		// First try to load block data from save
		const auto SaveManager = AMGameMode::GetSaveManager(this);
		if (const auto BlockSD = SaveManager->GetBlockData(BlockIndex))
		{
			// Regeneration feature applies only if the block isn't constant. Otherwise it must be loaded if save is present
			if (BlockSD->ConstantActorsCount > 0 || !bRegenerationFeature) //TODO: Implement this!!! Very important
			{
				SaveManager->TryLoadBlock(BlockIndex, this);
				return;
			}
		}

		// There was either no save data or saved block wasn't constant. Generate new contents for it
		// WE DON'T EMPTY THE BLOCK, allowing existing dynamic or const objects to remain
		BlockGenerator->SpawnActorsRandomly(BlockIndex, this, BlockMetadata);
	}
}

void AMWorldGenerator::RegenerateBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects, bool IgnoreConstancy)
{
	//TODO: refactor this function. Seems like it is redundant
	const auto BlockMetadata = AMGameMode::GetMetadataManager(this)->FindOrAddBlock(BlockIndex);
	if (!IgnoreConstancy && BlockMetadata->ConstantActorsCount > 0)
	{
		return;
	}
	const auto Block = EmptyBlock(BlockIndex, KeepDynamicObjects);
	BlockGenerator->SpawnActorsRandomly(BlockIndex, this, Block);
}

void AMWorldGenerator::BeginPlay()
{
	Super::BeginPlay();

	playerCount = 0; // TODO: Remove this temp hack for offline testing

	// Create the Block Generator
	BlockGenerator = BlockGeneratorBPClass ? NewObject<UMBlockGenerator>(GetOuter(), BlockGeneratorBPClass, TEXT("BlockGenerator")) : nullptr;
	check(BlockGenerator);

	// We want to set the biome coloring since the first block change
	BlocksPassedSinceLastPerimeterColoring = BiomesPerimeterColoringRate;
}

void AMWorldGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	AMGameMode::GetSaveManager(this)->SaveToMemory(this);
}

void AMWorldGenerator::CheckDynamicActorsBlocks()
{
	//TODO: TEST FOR EXCEPTIONS!
	if (ActiveBlocksMap.IsEmpty())
	{
		check(false);
		return;
	}

	// We cannot remove TMap elements during the iteration,
	// so that we remember all the transitions in the temporary array
	USTRUCT()
	struct FTransition
	{
		UPROPERTY()
		UActorWorldMetadata* ActorMetadata;
		FIntPoint OldBlockIndex;
		TOptional<FIntPoint> NewBlockIndex; // unset means that actor was removed
	};
	TMap<FName, FTransition> TransitionList;

	const auto MetadataManager = AMGameMode::GetMetadataManager(this);

	for (const auto& Index : ActiveBlocksMap)
	{
		const auto Block = MetadataManager->FindOrAddBlock(Index);
		for (const auto& [Name, Data] : Block->DynamicActors)
		{
			if (auto* ActorMetadata = MetadataManager->Find(Name))
			{
				if (!ActorMetadata->Actor) //TODO: Remove this temporary solution
				{
					// If the metadata has invalid Actor pointer, just delete this record
					const FTransition NewTransition{ActorMetadata, Index};
					TransitionList.Add(Name, NewTransition);
				}
				else
				{
					if (const auto ActualBlockIndex = GetGroundBlockIndex(Data->GetTransform().GetLocation());
						ActorMetadata->GroundBlockIndex != ActualBlockIndex)
					{
						const FTransition NewTransition{ActorMetadata, Index, ActualBlockIndex};
						TransitionList.Add(Name, NewTransition);
					}
				}
			}
		}
	}

	// Do all the remembered transitions
	for (const auto& [Name, Transition] : TransitionList)
	{
		if (Transition.NewBlockIndex.IsSet())
		{
			MetadataManager->MoveToBlock(Name, Transition.NewBlockIndex.GetValue());

			// Even though the dynamic object is still enabled, it might have moved to the disabled block (or even not generated yet),
			// where all surrounding static objects are disabled.
			// Check the environment for validity if you bind to the delegate!
			Transition.ActorMetadata->OnBlockChangedDelegate.Broadcast(Transition.OldBlockIndex, Transition.ActorMetadata->GroundBlockIndex);

			const auto RoadManager = AMGameMode::GetRoadManager(this);
			//Chunk transition check
			const auto OldChunk = RoadManager->GetChunkIndexByBlock(Transition.OldBlockIndex);
			const auto ActualChunk = RoadManager->GetChunkIndexByBlock(Transition.ActorMetadata->GroundBlockIndex);
			if (OldChunk != ActualChunk)
			{
				Transition.ActorMetadata->OnChunkChangedDelegate.Broadcast(OldChunk, ActualChunk);
			}
		}
		else
		{
			MetadataManager->Remove(Name);
		}
	}
}

void AMWorldGenerator::UpdateActiveZone(const FIntPoint& CenterBlock)
{
	const auto World = GetWorld();
	if (!IsValid(World)) return;

	// Enable all the objects within PlayerActiveZone. ActiveBlocksMap is considered as from the previous check.
	TSet<FIntPoint> ActiveBlocksMap_New;

	const auto MetadataManager = AMGameMode::GetMetadataManager(this);

	//TODO: Consider spreading the block logic over multiple ticks as done in OnTickGenerateBlocks()
	for (const auto BlockIndex : GetBlocksInRadius(CenterBlock.X, CenterBlock.Y, ActiveZoneRadius)) // you can add +1 to the ActiveZoneRadius if you need to see how the perimeter is generated in PIE
	{
		ActiveBlocksMap_New.Add(BlockIndex);
		ActiveBlocksMap.Remove(BlockIndex);
		auto* BlockMetadata = MetadataManager->FindOrAddBlock(BlockIndex);

		// Enable all the static Actors in the block
		for (const auto& [Index, Data] : BlockMetadata->StaticActors)
		{
			if (!Data)
			{ //TODO: Add a function to check the Data for nullptr and if yes then remove the record from StaticActors and ActorsMetadata
				check(false);
				continue;
			}
			if (const auto IsActiveCheckerComponent = Data->FindComponentByClass<UMIsActiveCheckerComponent>())
			{
				IsActiveCheckerComponent->EnableOwner();
			}
		}
		// Enable all dynamic Actors in the block
		for (const auto& [Index, Data] : BlockMetadata->DynamicActors)
		{
			if (!Data)
			{
				check(false);
				continue;
			}
			if (const auto IsActiveCheckerComponent = Data->FindComponentByClass<UMIsActiveCheckerComponent>())
			{
				IsActiveCheckerComponent->EnableOwner();
			}
		}
	}

	// Disable/Remove all the rest of objects that were in PlayerActiveZone in the previous check but no longer there.
	for (const auto& BlockIndex : ActiveBlocksMap)
	{
		if (const auto* GridBlock = MetadataManager->FindBlock(BlockIndex))
		{
			for (const auto& [Name, Data] : GridBlock->StaticActors)
			{
				if (!Data)
				{ //TODO: Add a function to check the Data for nullptr and if yes then remove the record from StaticActors and ActorsMetadata
					check(false);
					continue;
				}
				if (const auto IsActiveCheckerComponent = Data->FindComponentByClass<UMIsActiveCheckerComponent>())
				{
					IsActiveCheckerComponent->DisableOwner();
				}
			}
			for (const auto& [Name, Data] : GridBlock->DynamicActors)
			{
				if (!Data)
				{
					check(false);
					continue;
				}
				if (const auto IsActiveCheckerComponent = Data->FindComponentByClass<UMIsActiveCheckerComponent>())
				{
					IsActiveCheckerComponent->DisableOwner();
				}
			}
		}
		else check(false);
	}

	// All and only ActiveBlocksMap_New forms the new ActiveBlocksMap collection.
	ActiveBlocksMap = ActiveBlocksMap_New;
}

void AMWorldGenerator::UpdateNavigationMesh()
{
	if (const auto pWorld = GetWorld())
	{
		if (const auto PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			if (const auto NavMeshVolume = Cast<ANavMeshBoundsVolume>(UGameplayStatics::GetActorOfClass(pWorld, ANavMeshBoundsVolume::StaticClass())))
			{
				NavMeshVolume->SetActorLocation(PlayerPawn->GetTransform().GetLocation());

				if (const auto NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
				{
					NavSystem->OnNavigationBoundsUpdated(NavMeshVolume);
				}
				else
				{
					check(false);
				}
			}
		}
	}
}

//     x     
//  xxx xxx  
// xx     xx 
// x       x 
// x       x 
//x    0    x
// x       x 
// x       x 
// xx     xx 
//  xxx xxx  
//     x

void AMWorldGenerator::OnPlayerChangedBlock(const FIntPoint& IN_OldBlockIndex, const FIntPoint& IN_NewBlockIndex)
{
	if (bPendingTeleport)
	{
		//TODO: Handle teleport case
	}

	// If the next block is not adjacent (due to lag/low fps/very high player speed)
	// we recreate the continuous path travelled and generate perimeter for each travelled block
	auto OldBlockIndex = IN_OldBlockIndex;
	while(OldBlockIndex != IN_NewBlockIndex)
	{
		// Do increments separately for X and Y to prevent corner cutting. It is crucial for perimeter generation to avoid skipping tiles
		const auto XIncrement = FMath::Sign(IN_NewBlockIndex.X - OldBlockIndex.X);
		if (XIncrement != 0)
		{
			OldBlockIndex.X += XIncrement;
			TravelledDequeue.Add(OldBlockIndex);
		}
		const auto YIncrement = FMath::Sign(IN_NewBlockIndex.Y - OldBlockIndex.Y);
		if (YIncrement != 0)
		{
			OldBlockIndex.Y += YIncrement;
			TravelledDequeue.Add(OldBlockIndex);
		}
	}

	for (const auto& Block : TravelledDequeue)
	{
		UpdateActiveZone(Block);
		GenerateNewPieceOfPerimeter(Block);
	}
	TravelledDequeue.Empty();

	UpdateNavigationMesh();
}

void AMWorldGenerator::GenerateNewPieceOfPerimeter(const FIntPoint& CenterBlock)
{
	auto pWorld = GetWorld();
	if (!pWorld)
		return;

	auto NewPerimeter = GetBlocksOnPerimeter(CenterBlock.X, CenterBlock.Y, ActiveZoneRadius + 1);
	SetBiomesForBlocks(CenterBlock, NewPerimeter);
	PendingBlocks.Append(NewPerimeter);

	auto TopLeftScreenPointInWorld = RaycastScreenPoint(pWorld, EScreenPoint::TopLeft);
	auto TopRighScreenPointInWorld = RaycastScreenPoint(pWorld, EScreenPoint::TopRight);
	PendingBlocks.Sort([this, &TopLeftScreenPointInWorld, &TopRighScreenPointInWorld](const FIntPoint& BlockA, const FIntPoint& BlockB)
	{ // Sort blocks so that the closest to the screen corners would be the first
		const auto LocationA = GetGroundBlockLocation(BlockA);
		const auto LocationB = GetGroundBlockLocation(BlockB);

		const auto MinDistanceToScreenEdgesA = FMath::Min(FVector::Distance(LocationA, TopLeftScreenPointInWorld), FVector::Distance(LocationA, TopRighScreenPointInWorld));
		const auto MinDistanceToScreenEdgesB = FMath::Min(FVector::Distance(LocationB, TopLeftScreenPointInWorld), FVector::Distance(LocationB, TopRighScreenPointInWorld));

		return MinDistanceToScreenEdgesA <= MinDistanceToScreenEdgesB;
	});

	// We'll spread heavy GenerateBlock calls over the next few ticks
	OnTickGenerateBlocks();
}

/** Function to calculate the angle between [0; 1] vector and the vector from O to P */
float GetAngle(const FIntPoint& O, const FIntPoint& P)
{
	const FIntPoint OP = P - O;
	const FVector2D OPFloat(OP.X, OP.Y);
	const FVector2D ReferenceVector(0, 1);
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector2D::DotProduct(OPFloat.GetSafeNormal(), ReferenceVector)));
	return OP.X < 0 ? 360.0f - Angle : Angle;
}

TArray<FBiomeDelimiter> Delimiters;
void AMWorldGenerator::SetBiomesForBlocks(const FIntPoint& CenterBlock, TSet<FIntPoint>& BlocksToGenerate)
{
	check(!BlocksToGenerate.IsEmpty())
	++BlocksPassedSinceLastPerimeterColoring;

	// Sort the blocks in ascending order of the polar angle
	BlocksToGenerate.Sort([&CenterBlock](const FIntPoint& BlockA, const FIntPoint& BlockB) {
		return GetAngle(CenterBlock, BlockA) < GetAngle(CenterBlock, BlockB);
	});

	// Set biomes for the generation perimeter. They will remain same for specified number of blocks, providing us with variety of biomes forms 
	if (BlocksPassedSinceLastPerimeterColoring >= BiomesPerimeterColoringRate)
	{
		BlocksPassedSinceLastPerimeterColoring = 0;

		// Number of biome types on the generation perimeter for the current coloring
		int BiomesNumberInCurrentColoring = StaticEnum<EBiome>()->NumEnums() - 1; // - 1 because of the implicit MAX value

		// We split all the blocks into 'BiomesNumberInCurrentColoring' number of sub-arrays with random length.
		// Delimiters denote the ending of the sub-arrays
		Delimiters.Empty();

		// Simple coloring. Split perimeter into 3 equal segments of random colors (may have duplicates)
		int LastDelimiterBlock = BlocksToGenerate.Num() / BiomesNumberInCurrentColoring;
		for (int i = 0; i < BiomesNumberInCurrentColoring - 1; ++i)
		{
			Delimiters.Add({LastDelimiterBlock, static_cast<EBiome>(FMath::RandRange(0, BiomesNumberInCurrentColoring - 1))});
			LastDelimiterBlock += BlocksToGenerate.Num() / BiomesNumberInCurrentColoring;
		}
		Delimiters.Add({BlocksToGenerate.Num() - 1, static_cast<EBiome>(BiomesNumberInCurrentColoring - 1)});
	}

	if (!Delimiters.IsEmpty())
	{
		int DelimiterIndex = 0;
		int BlockIndex = 0;
		// Set biomes for blocks
		for (auto Block : BlocksToGenerate)
		{
			if (BlockIndex > Delimiters[DelimiterIndex].BlockPosition)
			{
				++DelimiterIndex;
			}

			const auto BlockMetadata = AMGameMode::GetMetadataManager(this)->FindOrAddBlock(Block);

			// So far, biome constancy doesn't look very good
			//if (BlockMetadata->ConstantActorsCount <= 0) // We keep the biome as well as all other objects
			{
				BlockMetadata->Biome = Delimiters[DelimiterIndex].Biome;
			}
			++BlockIndex;
		}
	}
}

//TODO: Make asynchronous
void AMWorldGenerator::OnTickGenerateBlocks()
{
	constexpr int BlocksPerFrame = 1;
	int Index = 0;
	while (!PendingBlocks.IsEmpty())
	{
		auto It = PendingBlocks.CreateIterator();
		LoadOrGenerateBlock(*It);
		It.RemoveCurrent();

		if (++Index >= BlocksPerFrame)
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick([this]{ OnTickGenerateBlocks(); });
			return;
		}
	}
}

FIntPoint AMWorldGenerator::GetPlayerGroundBlockIndex() const
{
	if (const auto pPlayerMetadata = AMGameMode::GetMetadataManager(this)->Find("Player"))
	{
		return GetGroundBlockIndex(pPlayerMetadata->Actor->GetActorLocation());
	}
	check(false);
	return {};
}

FVector AMWorldGenerator::GetGroundBlockSize() const
{
	if (const auto ToSpawnGroundBlock = ToSpawnActorClasses.Find(FName("GroundBlock")); GetWorld())
	{
		const auto GroundBlockBounds = GetDefaultBounds(ToSpawnGroundBlock->Get(), GetWorld());
		return GroundBlockBounds.BoxExtent * 2.f;
	}
	check(false);
	return FVector::ZeroVector;
}

FIntPoint AMWorldGenerator::GetGroundBlockIndex(FVector Position) const
{
	const auto GroundBlockSize = GetGroundBlockSize();
	if (GroundBlockSize.IsZero())
	{
		check(false);
		return FIntPoint::ZeroValue;
	}
	return FIntPoint(FMath::FloorToInt(Position.X/GroundBlockSize.X), FMath::FloorToInt(Position.Y/GroundBlockSize.Y));
}

FVector AMWorldGenerator::GetGroundBlockLocation(FIntPoint BlockIndex)
{
	const auto GroundBlockSize = GetGroundBlockSize();
	return FVector(BlockIndex.X * GroundBlockSize.X, BlockIndex.Y * GroundBlockSize.Y, 0.);
}

static bool RayPlaneIntersection(const FVector& RayOrigin, const FVector& RayDirection, float PlaneZ, FVector& IntersectionPoint)
{
	if (FMath::IsNearlyZero(RayDirection.Z))
	{
		// The ray is parallel to the plane
		return false;
	}

	float t = (PlaneZ - RayOrigin.Z) / RayDirection.Z;
	if (t < 0)
	{
		// Intersection point is behind the camera
		return false;
	}

	IntersectionPoint = RayOrigin + RayDirection * t;
	return true;
}

FVector AMWorldGenerator::RaycastScreenPoint(const UObject* pWorldContextObject, const EScreenPoint ScreenPoint)
{
	auto CameraManager = UGameplayStatics::GetPlayerCameraManager(pWorldContextObject, 0);

	if (CameraManager)
	{
		FVector CameraLocation = CameraManager->GetCameraLocation();
		FRotator CameraRotation = CameraManager->GetCameraRotation();

		FVector2D ScreenSize;
		GEngine->GameViewport->GetViewportSize(ScreenSize);

		FVector WorldDirection;
		FVector2D ScreenPosition = ScreenPoint == EScreenPoint::TopLeft ? FVector2D::ZeroVector : FVector2D(ScreenSize.X, 0);
		UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(pWorldContextObject, 0), ScreenPosition, CameraLocation, WorldDirection);

		FVector IntersectionPoint;
		RayPlaneIntersection(CameraLocation, WorldDirection, 0, IntersectionPoint);
		return IntersectionPoint;
	}
	return FVector::ZeroVector;
}

void AMWorldGenerator::DrawDebuggingInfo() const
{
	FlushPersistentDebugLines(GetWorld());
	if (const auto RoadManager = AMGameMode::GetRoadManager(this))
	{
		if (const auto GroundMarker = RoadManager->GetGroundMarker())
		{
			GroundMarker->Render();
		}
	}
}

bool IsOnPerimeter(int x, int y, int centerX, int centerY, int radius) {
	// Calculate the distance of the square's center from the circle's center
	float dist = std::sqrt(std::pow(x + 0.5 - centerX, 2) + std::pow(y + 0.5 - centerY, 2));

	// Check if the square's center is within the circle defined by the radius
	// but not in a smaller circle to exclude corners
	return dist < radius && dist > radius - std::sqrt(2);
}

TSet<FIntPoint> AMWorldGenerator::GetBlocksOnPerimeter(int CenterX, int CenterY, int Radius)
{ // Bresenham's algorithm
	TSet<FIntPoint>perimeterPoints;
	int x = Radius;
	int y = 0;
	int err = 0;

	while (x >= y) {
		// Top-right quadrant
		perimeterPoints.Add({CenterX + x, CenterY + y});
		perimeterPoints.Add({CenterX + y, CenterY + x});

		// Top-left quadrant
		perimeterPoints.Add({CenterX - y, CenterY + x});
		perimeterPoints.Add({CenterX - x, CenterY + y});

		// Bottom-left quadrant
		perimeterPoints.Add({CenterX - x, CenterY - y});
		perimeterPoints.Add({CenterX - y, CenterY - x});

		// Bottom-right quadrant
		perimeterPoints.Add({CenterX + y, CenterY - x});
		perimeterPoints.Add({CenterX + x, CenterY - y});

		y += 1;
		err += 1 + 2 * y;
		if (2 * (err - x) + 1 > 0) {
			x -= 1;
			err += 1 - 2 * x;
		}
	}

	return perimeterPoints;
}

TSet<FIntPoint> AMWorldGenerator::GetBlocksInRadius(int CenterX, int CenterY, int Radius)
{
	Radius += 1;
	TSet<FIntPoint> InternalSquares;
	// Define the size of the grid
	int GridSize = 2 * Radius + 1;
	std::vector<std::vector<bool>> Grid(GridSize, std::vector<bool>(GridSize, false));

	// Mark the perimeter squares
	TSet<FIntPoint> PerimeterSquares = GetBlocksOnPerimeter(CenterX, CenterY, Radius);
	for (const auto& Square : PerimeterSquares) {
		Grid[Square.X - (CenterX - Radius)][Square.Y - (CenterY - Radius)] = true;
	}

	// Helper function for flood fill
	std::function<void(int, int)> Fill = [&](int x, int y) {
		if (x < 0 || x >= GridSize || y < 0 || y >= GridSize || Grid[x][y]) return;
		Grid[x][y] = true;
		InternalSquares.Add({x + (CenterX - Radius), y + (CenterY - Radius)});
		Fill(x + 1, y);
		Fill(x - 1, y);
		Fill(x, y + 1);
		Fill(x, y - 1);
	};

	// Start flood fill from the center
	Fill(Radius, Radius);

	return InternalSquares;
}

TSubclassOf<AActor> AMWorldGenerator::GetActorClassToSpawn(FName Name)
{
	const auto Result = ToSpawnActorClasses.Find(Name);
	if (Result)
		return *Result;
	return nullptr;
}

void AMWorldGenerator::SetupInputComponent()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		EnableInput(PC);
		if (const auto RoadManager = AMGameMode::GetRoadManager(this))
		{
			if (const auto GroundMarker = RoadManager->GetGroundMarker())
			{
				InputComponent->BindAction("ToggleDebuggingGeometry", IE_Released, GroundMarker,
										   &UMGroundMarker::OnToggleDebuggingGeometry);
			}
		}
	}
}

// Tick interval is set in the blueprint
void AMWorldGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckDynamicActorsBlocks();

	DrawDebuggingInfo();
}

AActor* AMWorldGenerator::SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation,
                                     const FActorSpawnParameters& SpawnParameters, bool bForceAboveGround, const FOnSpawnActorStarted& OnSpawnActorStarted, const FMUid& Uid)
{
	const auto pWorld = GetWorld();
	if (!pWorld || !Class)
	{
		check(false);
		return nullptr;
	}

	FTransform ActorTransform;

	if (bForceAboveGround)
	{
		// Raise the actor until the bottom point is above the ground
		auto ActorBounds = GetDefaultBounds(Class, pWorld);
		ActorBounds.Origin += Location;
		auto LocationAboveGround = Location;
		if (ActorBounds.Origin.Z - ActorBounds.BoxExtent.Z < 0.f)
		{
			LocationAboveGround.Z -= ActorBounds.Origin.Z - ActorBounds.BoxExtent.Z;
		}
		ActorTransform = FTransform(Rotation, LocationAboveGround);
	}
	else
	{
		ActorTransform = FTransform(Rotation, Location);
	}

	AActor* Actor;
	if (OnSpawnActorStarted.IsBound())
	{
		//TODO: Figure out how to support SpawnParameters.Name
		Actor = pWorld->SpawnActorDeferred<AActor>(
			Class,
			ActorTransform,
			nullptr,
			nullptr,
			SpawnParameters.SpawnCollisionHandlingOverride,
			SpawnParameters.TransformScaleMethod
		);

		check(Actor);
		if (Actor)
		{
			if (!SpawnParameters.Name.IsNone())
				Actor->Rename(*SpawnParameters.Name.ToString());
			/*else
				Actor->Rename(); // Guarantees unique name*/

			OnSpawnActorStarted.Broadcast(Actor);

			UGameplayStatics::FinishSpawningActor(Actor, ActorTransform);
		}
	}
	else
	{
		Actor = pWorld->SpawnActor<AActor>(Class, ActorTransform, SpawnParameters);
	}

	if (!Actor)
	{
		check(false);
		return nullptr;
	}

	EnrollActorToGrid(Actor, Uid);

	const auto ExperienceManager = AMGameMode::GetExperienceManager(this);
	if (const auto PickableActor = Cast<AMPickableActor>(Actor); PickableActor && ExperienceManager)
	{ // Enroll pickable actor to experience manager
		PickableActor->PickedUpCompletelyDelegate.AddDynamic(ExperienceManager, &UMExperienceManager::OnActorPickedUp);
	}

	return Actor;
}

//TODO:Move this to MetadataManager
void AMWorldGenerator::EnrollActorToGrid(AActor* Actor, const FMUid& Uid)
{
	if (!Actor)
	{
		check(false);
		return;
	}

	const auto GroundBlockIndex = GetGroundBlockIndex(Actor->GetActorLocation());

	const auto MetadataManager = AMGameMode::GetMetadataManager(this);

	// If there's an empty block, add it to the map
	const auto BlockMetadata = MetadataManager->FindOrAddBlock(GroundBlockIndex);

	// Determine whether the object is static or movable
	auto& ListToAdd = UMMetadataManager::IsDynamic(Actor) ? BlockMetadata->DynamicActors : BlockMetadata->StaticActors;
	ListToAdd.Add(FName(Actor->GetName()), Actor);

	const auto UidChecked = IsUidValid(Uid) ? Uid : AMGameMode::GetSaveManager(this)->GenerateUid();

	// Store actor metadata
	MetadataManager->Add(FName(Actor->GetName()), Actor, UidChecked, GroundBlockIndex);

	// If was spawned on a disabled block, disable the actor (unless the actor has to be always enabled)
	if (const auto IsActiveCheckerComponent = Cast<UMIsActiveCheckerComponent>(Actor->GetComponentByClass(UMIsActiveCheckerComponent::StaticClass())))
	{
		if (IsActiveCheckerComponent->GetAlwaysEnabled() || IsActiveCheckerComponent->GetPreserveBlockConstancy())
		{
			BlockMetadata->ConstantActorsCount++; //TODO: Disable constancy when the object no longer on the block
		}
		if (!ActiveBlocksMap.Contains(GroundBlockIndex) && !IsActiveCheckerComponent->GetAlwaysEnabled())
		{
			IsActiveCheckerComponent->DisableOwner();
		}
	}
}

TSubclassOf<AActor> AMWorldGenerator::GetClassToSpawn(FName Name)
{
	if (const auto Class = ToSpawnActorClasses.Find(Name))
	{
		return *Class;
	}
	return nullptr;
}

TMap<FName, AActor*> AMWorldGenerator::GetActorsInRect(FVector UpperLeft, FVector BottomRight, bool bDynamic)
{
	const auto StartBlock = GetGroundBlockIndex(UpperLeft);
	const auto FinishBlock = GetGroundBlockIndex(BottomRight);

	TMap<FName, AActor*> Result;

	const auto MetadataManager = AMGameMode::GetMetadataManager(this);

	for (auto X = StartBlock.X; X <= FinishBlock.X; ++X)
	{
		for (auto Y = StartBlock.Y; Y <= FinishBlock.Y; ++Y)
		{
			if (const auto Block = MetadataManager->FindBlock({X, Y}))
			{
				if (const auto& Actors = bDynamic ? Block->DynamicActors : Block->StaticActors;
					!Actors.IsEmpty())
				{
					for (const auto& [Name, Actor] : Actors)
					{
						if (const auto Metadata = MetadataManager->Find(Name); Actor)
						{
							Result.Add(Name, Metadata->Actor);
						}
					}
				}
			}
		}
	}

	return Result;
}

/*void AMWorldGenerator::CleanArea(const FVector& Location, int RadiusInBlocks, UPCGGraph* OverridePCGGraph)
{
	const auto CenterBlock = GetGroundBlockIndex(Location);
	for (const auto Block : GetBlocksInRadius(CenterBlock.X, CenterBlock.Y, RadiusInBlocks))
	{
		const auto BlockMetadata = EmptyBlock(Block, true);
		if (OverridePCGGraph)
		{
			BlockMetadata->PCGGraph = OverridePCGGraph;
		}
	}
}*/

void AMWorldGenerator::RegenerateArea(const FVector& Location, int RadiusInBlocks, UPCGGraph* OverridePCGGraph)
{
	//TODO: Reuse CleanArea for this. Probably overload CleanArea() to take a TSet<FIntPoint>
	const auto CenterBlock = GetGroundBlockIndex(Location);
	for (const auto Block : GetBlocksInRadius(CenterBlock.X, CenterBlock.Y, RadiusInBlocks))
	{
		const auto BlockMetadata = AMGameMode::GetMetadataManager(this)->FindOrAddBlock(Block);
		if (OverridePCGGraph)
		{
			BlockMetadata->PCGGraph = OverridePCGGraph;
		}
		RegenerateBlock(Block, true, false);
	}
}

/** Finds the bounds of the default object for a blueprint. You have to mark components with AffectsDefaultBounds tag in order to affect bounds! */
FBoxSphereBounds AMWorldGenerator::GetDefaultBounds(UClass* IN_ActorClass, UObject* WorldContextObject)
{
	FBoxSphereBounds ActorBounds(ForceInitToZero);

	if (const auto WorldGenerator = AMGameMode::GetWorldGenerator(WorldContextObject))
	{
		if (const auto FoundBounds = WorldGenerator->DefaultBoundsMap.Find(IN_ActorClass))
		{
			return *FoundBounds;
		}

		if (!IN_ActorClass)
		{
			check(false)
			return ActorBounds;
		}

		const auto _ = AMGameMode::GetMetadataManager(WorldContextObject)->FindOrAddBlock(FIntPoint::ZeroValue);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = FName("TestBounds_" + IN_ActorClass->GetName());
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (const auto Actor = WorldContextObject->GetWorld()->SpawnActorDeferred<AActor>(IN_ActorClass, FTransform::Identity))
		{
			Actor->SetReplicates(false); // This might cause problems since the actor hasn't Begun Play. Be cautious especially using Iris
			Actor->Tags.Add("DummyForDefaultBounds");
			UGameplayStatics::FinishSpawningActor(Actor, FTransform::Identity);
			Actor->SetActorEnableCollision(false);

			// Calculate the Actor bounds by accumulating the bounds of its components
			FBox ActorBox(EForceInit::ForceInitToZero);
			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
				{
					if (PrimitiveComponent->ComponentHasTag("AffectsDefaultBounds") ||
					(Cast<UStaticMeshComponent>(Component) != nullptr && !PrimitiveComponent->ComponentHasTag("IgnoreDefaultBounds")))
					{
						FTransform ComponentTransform = PrimitiveComponent->GetComponentTransform();
						FBoxSphereBounds ComponentBounds = PrimitiveComponent->CalcBounds(ComponentTransform);
						ActorBox += ComponentBounds.GetBox();
					}
				}
			}

			ActorBounds.Origin = ActorBox.GetCenter();
			ActorBounds.BoxExtent = ActorBox.GetExtent();
			ActorBounds.SphereRadius = ActorBox.GetExtent().Size2D();
			WorldGenerator->DefaultBoundsMap.Add(IN_ActorClass, ActorBounds);

			Actor->Destroy();
		}
	}

	return ActorBounds;
}

AActor* AMWorldGenerator::SpawnActorInRadius(UClass* Class, const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters, const float ToSpawnRadius, const float ToSpawnHeight, const FOnSpawnActorStarted& OnSpawnActorStarted)
{
	const auto pWorld = GetWorld();
	if (!pWorld)
		return nullptr;

	const auto DefaultBounds = GetDefaultBounds(Class, pWorld);
	const auto BoundsRadius = FMath::Max(DefaultBounds.BoxExtent.X, DefaultBounds.BoxExtent.Y);

	if (FMath::IsNearlyZero(BoundsRadius))
		return nullptr;

	const int TriesNumber = 2 * PI * ToSpawnRadius / BoundsRadius;
	TArray<float> AnglesToTry;
	const auto StartAngle = FMath::FRandRange(0.f, 360.f);

	for (int AngleIndex = 0; AngleIndex < TriesNumber; ++AngleIndex)
	{
		const float AngleToTry = StartAngle + 360.f * AngleIndex / TriesNumber;
		const FVector SpawnPositionOffset (
				ToSpawnRadius * FMath::Cos(FMath::DegreesToRadians(AngleToTry)),
				ToSpawnRadius * FMath::Sin(FMath::DegreesToRadians(AngleToTry)),
				0.f
			);

		FVector SpawnPosition = Location + SpawnPositionOffset;
		SpawnPosition.Z = ToSpawnHeight;

		if (const auto Actor = SpawnActor<AActor>(Class, SpawnPosition, Rotation, SpawnParameters, true, OnSpawnActorStarted))
		{
			return Actor;
		}
	}

	if (ToSpawnRadius >= 1000.f) // dummy check
	{
		check(false);
		return nullptr;
	}

	// If check is failed, consider incrementing ToSpawnRadius
	return SpawnActorInRadius(Class, Location, Rotation, SpawnParameters, ToSpawnRadius + BoundsRadius * 2.f, ToSpawnHeight, OnSpawnActorStarted);
}
