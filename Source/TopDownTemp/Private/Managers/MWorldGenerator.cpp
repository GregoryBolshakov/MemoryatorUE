// Fill out your copyright notice in the Description page of Project Settings.

#include "MWorldGenerator.h"

#include "Math/UnrealMathUtility.h"
#include "StationaryActors/MActor.h" 
#include "Characters/MCharacter.h"
#include "StationaryActors/MGroundBlock.h"
#include "MBlockGenerator.h"
#include "MCommunicationManager.h"
#include "MDropManager.h"
#include "MReputationManager.h"
#include "MExperienceManager.h"
#include "MRoadManager.h"
#include "Components/MIsActiveCheckerComponent.h"
#include "MVillageGenerator.h"
#include "NavigationSystem.h"
#include "Controllers/MPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NakamaManager/Private/NakamaManager.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "StationaryActors/MPickableActor.h"
#include "MSaveManager.h"
#include "MWorldManager.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/SplineMeshActor.h"
#include "StationaryActors/MRoadSplineActor.h"

AMWorldGenerator::AMWorldGenerator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}
//temp
FTimerHandle tempTimer, tempTimer2;
void AMWorldGenerator::InitNewWorld()
{
	//TODO: Erase the old code. Now we consider this function to be called ONLY in the new empty world.
	auto* pWorld = GetWorld();
	if (!pWorld)
		return;

	// Spawn player and add it to the Grid
	APawn* pPlayer = nullptr;
	if (const auto pPlayerMetadata = ActorsMetadata.Find("Player"))
	{
		pPlayer = Cast<APawn>(pPlayerMetadata->Actor);
	}
	else
	{
		if (const auto PlayerClass = ToSpawnActorClasses.Find("Player"))
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Name = "Player";
			pPlayer = SpawnActor<AMCharacter>(*PlayerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams, true);
		}
	}
	if (!IsValid(pPlayer))
	{
		check(false);
		return;
	}
	if (const auto pPlayerController = UGameplayStatics::GetPlayerController(pWorld, 0))
		pPlayerController->Possess(pPlayer);
	else
		check(false);

	RoadManager->ConnectTwoChunks({-5, 0}, {5, 5});

	UpdateActiveZone(); // temp solution to avoid disabling all subsequently generated actors due to empty ActiveBlocksMap

	pWorld->GetTimerManager().SetTimer(tempTimer, [this, pWorld, pPlayer]()
		{
			const auto PlayerBlockIndex = GetGroundBlockIndex(pPlayer->GetTransform().GetLocation());

			if (GridOfActors.Num() == 1 || ActorsMetadata.Num() == 1) // Empty world check
				{
					check(GridOfActors.Num() == 1 && ActorsMetadata.Num() == 1); // Error occured while loading save

					// We add 1 to the radius on purpose. Generated area always has to be further then visible
					const auto BlocksInRadius = GetBlocksInRadius(PlayerBlockIndex.X, PlayerBlockIndex.Y, ActiveZoneRadius + 1);
					for (const auto BlockInRadius : BlocksInRadius)
					{ // Set the biomes in a separate pass first because we need to know each biome during block generation in order to disable/enable block transitions
						auto* BlockMetadata = GridOfActors.Get(BlockInRadius);
						if (!BlockMetadata)
						{
							BlockMetadata = GridOfActors.Add(BlockInRadius, NewObject<UBlockMetadata>(this));
						}
						BlockMetadata->Biome = BiomeForInitialGeneration;
					}
					for (const auto BlockInRadius : BlocksInRadius)
					{
						GenerateBlock(BlockInRadius);
					}

					pWorld->GetTimerManager().SetTimer(tempTimer2, [this, pWorld, pPlayer]()
					{
						const auto VillageClass = ToSpawnComplexStructureClasses.Find("Village")->Get();
						const auto VillageGenerator = pWorld->SpawnActor<AMVillageGenerator>(VillageClass, FVector::Zero(), FRotator::ZeroRotator);
						VillageGenerator->Generate();
						UpdateNavigationMesh();
					}, 0.2f, false);
				}
		}
	, 0.2f, false);

	/*EmptyBlock({PlayerBlockIndex.X, PlayerBlockIndex.Y}, true);
	BlockGenerator->SpawnActors({PlayerBlockIndex.X, PlayerBlockIndex.Y}, this, EBiome::BirchGrove, "TestBlock");*/
}

UBlockMetadata* AMWorldGenerator::EmptyBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects, bool IgnoreConstancy)
{
	const auto pWorld = GetWorld();
	if (!pWorld)
		return nullptr;

	// Get the block from grid or add if doesn't exist
	auto* BlockMetadata = GridOfActors.Get(BlockIndex);
	if (!BlockMetadata)
	{
		BlockMetadata = GridOfActors.Add(BlockIndex, NewObject<UBlockMetadata>(this));
	}

	if (BlockMetadata->ConstantActorsCount > 0 && !IgnoreConstancy)
		return BlockMetadata;

	// Empty the block if already spawned
	for (auto It = BlockMetadata->StaticActors.CreateIterator(); It; ++It)
	{
		if (It->Value)
		{
			RemoveActorFromGrid(It->Value); //TODO: Remake this. Removal during iteration
		}
	}
	check(BlockMetadata->StaticActors.IsEmpty());

	if (!KeepDynamicObjects)
	{
		for (auto It = BlockMetadata->DynamicActors.CreateIterator(); It; ++It)
		{
			if (It->Value)
			{
				RemoveActorFromGrid(It->Value); //TODO: Remake this. Removal during iteration
			}
		}
		check(BlockMetadata->DynamicActors.IsEmpty());
	}

	return BlockMetadata;
}

UBlockMetadata* AMWorldGenerator::GenerateBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects)
{
	const auto Block = EmptyBlock(BlockIndex, KeepDynamicObjects);
	BlockGenerator->SpawnActorsRandomly(BlockIndex, this, Block->Biome);
	return Block;
}

void AMWorldGenerator::BeginPlay()
{
	Super::BeginPlay();

	// Create the Drop Manager
	DropManager = DropManagerBPClass ? NewObject<UMDropManager>(GetOuter(), DropManagerBPClass, TEXT("DropManager")) : nullptr;
	check(DropManager);

	// Create the Reputation Manager
	ReputationManager = ReputationManagerBPClass ? NewObject<UMReputationManager>(GetOuter(), ReputationManagerBPClass, TEXT("ReputationManager")) : nullptr;
	if (ReputationManager)
	{
		ReputationManager->Initialize({{EFaction::Humans, {100, 10}}, {EFaction::Nightmares, {5, 4}}, {EFaction::Witches, {1, 0}}}); // temporary set manually
	}
	else
	{
		check(false);
	}

	ExperienceManager = ExperienceManagerBPClass ? NewObject<UMExperienceManager>(GetOuter(), ExperienceManagerBPClass, TEXT("ExperienceManager")) : nullptr;
	check(ExperienceManager);

	SaveManager = SaveManagerBPClass ? NewObject<UMSaveManager>(GetOuter(), SaveManagerBPClass, TEXT("SaveManager")) : nullptr;
	if (!IsValid(SaveManager))
	{
		check(false);
		return;
	}

	// Spawn the Communication Manager
	CommunicationManager = CommunicationManagerBPClass ? GetWorld()->SpawnActor<AMCommunicationManager>(CommunicationManagerBPClass) : nullptr;
	check(CommunicationManager);

	RoadManager = NewObject<UMRoadManager>(GetOuter(), UMRoadManager::StaticClass(), TEXT("RoadManager"));
	RoadManager->Initialize(this);
	check(RoadManager);

	// Create the Block Generator
	BlockGenerator = BlockGeneratorBPClass ? NewObject<UMBlockGenerator>(GetOuter(), BlockGeneratorBPClass, TEXT("BlockGenerator")) : nullptr;
	check(BlockGenerator);

	// We want to set the biome coloring since the first block change
	BlocksPassedSinceLastPerimeterColoring = BiomesPerimeterColoringRate;

	//if (!SaveManager->AsyncLoadFromMemory(this))
	{
		InitNewWorld();
	}
	UpdateActiveZone();

	// Bind to the player-moves-to-another-block event 
	if (const auto pPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		ExperienceManager->ExperienceAddedDelegate.AddDynamic(Cast<AMPlayerController>(pPlayer->GetController()), &AMPlayerController::OnExperienceAdded);
		if (const auto PlayerMetadata = ActorsMetadata.Find(FName(pPlayer->GetName())))
		{
			PlayerMetadata->OnBlockChangedDelegate.AddDynamic(this, &AMWorldGenerator::OnPlayerChangedBlock);
		}
	}

	SaveManager->SetUpAutoSaves(GridOfActors, this);
}

void AMWorldGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	SaveManager->SaveToMemory(GridOfActors, this);
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
		FActorWorldMetadata& ActorMetadata;
		FIntPoint OldBlockIndex;
		UBlockMetadata& OldBlock;
		UPROPERTY()
		UBlockMetadata* NewBlock;
	};
	TMap<FName, FTransition> TransitionList;

	for (const auto [Index, Bool] : ActiveBlocksMap)
	{
		if (!Bool)
		{
			check(false)
			continue;
		}
		if (const auto Block = GridOfActors.Get(Index); Block && !Block->DynamicActors.IsEmpty())
		{
			for (const auto& [Name, Data] : Block->DynamicActors)
			{
				if (const auto ActorMetadata = ActorsMetadata.Find(Name))
				{
					if (!ActorMetadata->Actor) //TODO: Remove this temporary solution
					{
						// If the metadata has invalid Actor pointer, just delete this record
						const FTransition NewTransition{*ActorMetadata, Index, *Block, nullptr};
						TransitionList.Add(Name, NewTransition);
					}
					else
					{
						if (const auto ActualBlockIndex = GetGroundBlockIndex(Data->GetTransform().GetLocation());
							ActorMetadata->GroundBlockIndex != ActualBlockIndex)
						{
							if (const auto NewBlock = GridOfActors.Get(ActualBlockIndex))
							{
								const FTransition NewTransition{*ActorMetadata, Index, *Block, NewBlock};
								TransitionList.Add(Name, NewTransition);
								ActorMetadata->GroundBlockIndex = ActualBlockIndex;
							}
						}
					}
				}
			}
		}
	}

	// Do all the remembered transitions
	for (const auto& [Name, Transition] : TransitionList)
	{
		Transition.OldBlock.DynamicActors.Remove(Name);
	}
	for (auto& [Name, Transition] : TransitionList)
	{
		if (Transition.NewBlock)
		{
			Transition.NewBlock->DynamicActors.Add(Name, Transition.ActorMetadata.Actor);

			// Even though the dynamic object is still enabled, it might have moved to the disabled block,
			// where all surrounding static objects are disabled.
			// Check the environment for validity if you bind to the delegate!
			Transition.ActorMetadata.OnBlockChangedDelegate.Broadcast(Transition.OldBlockIndex, Transition.ActorMetadata.GroundBlockIndex);
		}
	}
}

void AMWorldGenerator::UpdateActiveZone()
{
	const auto World = GetWorld();
	if (!IsValid(World)) return;
	const auto Player = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(Player)) return;

	const auto PlayerLocation = Player->GetTransform().GetLocation();
	const auto PlayerBlock = GetGroundBlockIndex(PlayerLocation);

	// Enable all the objects within PlayerActiveZone. ActiveBlocksMap is considered as from the previous check.
	TMap<FIntPoint, bool> ActiveBlocksMap_New;

	//TODO: Consider spreading the block logic over multiple ticks as done in OnTickGenerateBlocks()
	for (const auto Block : GetBlocksInRadius(PlayerBlock.X, PlayerBlock.Y, ActiveZoneRadius)) // you can add +1 to the ActiveZoneRadius if you need to see how the perimeter is generated in PIE
	{
		ActiveBlocksMap_New.Add(Block, true);
		ActiveBlocksMap.Remove(Block);
		if (const auto GridBlock = GridOfActors.Get(Block))
		{
			// Enable all the static Actors in the block
			for (const auto& [Index, Data] : GridBlock->StaticActors)
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
			for (const auto& [Index, Data] : GridBlock->DynamicActors)
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
	}

	// Disable/Remove all the rest of objects that were in PlayerActiveZone in the previous check but no longer there.
	for (const auto& [BlockIndex, IsActive] : ActiveBlocksMap)
	{
		if (const auto GridBlock = GridOfActors.Get(BlockIndex))
		{
			for (const auto& [Index, Data] : GridBlock->StaticActors)
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
			for (const auto& [Index, Data] : GridBlock->DynamicActors)
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
		//TODO: If the block isn't constant and the GridOfActors is almost full, delete the furthest block and destroy its actors
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
		GenerateNewPieceOfPerimeter(Block);
	}
	TravelledDequeue.Empty();

	UpdateActiveZone();
	UpdateNavigationMesh();
}

void AMWorldGenerator::GenerateNewPieceOfPerimeter(const FIntPoint& CenterBlock)
{
	auto pWorld = GetWorld();
	if (!pWorld)
		return;

	auto BlocksOnPerimeter = GetBlocksOnPerimeter(CenterBlock.X, CenterBlock.Y, ActiveZoneRadius + 1);

	SetBiomesForBlocks(CenterBlock, BlocksOnPerimeter);
	RoadManager->GenerateNewPieceForRoads(BlocksOnPerimeter);

	auto TopLeftScreenPointInWorld = RaycastScreenPoint(pWorld, EScreenPoint::TopLeft);
	auto TopRighScreenPointInWorld = RaycastScreenPoint(pWorld, EScreenPoint::TopRight);
	BlocksOnPerimeter.Sort([this, &TopLeftScreenPointInWorld, &TopRighScreenPointInWorld](const FIntPoint& BlockA, const FIntPoint& BlockB)
	{ // Sort blocks so that the closest to the screen corners would be the first
		const auto LocationA = GetGroundBlockLocation(BlockA);
		const auto LocationB = GetGroundBlockLocation(BlockB);

		const auto MinDistanceToScreenEdgesA = FMath::Min(FVector::Distance(LocationA, TopLeftScreenPointInWorld), FVector::Distance(LocationA, TopRighScreenPointInWorld));
		const auto MinDistanceToScreenEdgesB = FMath::Min(FVector::Distance(LocationB, TopLeftScreenPointInWorld), FVector::Distance(LocationB, TopRighScreenPointInWorld));

		return MinDistanceToScreenEdgesA <= MinDistanceToScreenEdgesB;
	});

	// We'll spread heavy GenerateBlock calls over the next few ticks
	OnTickGenerateBlocks(BlocksOnPerimeter);
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

			auto* BlockMetadata = GridOfActors.Get(Block);
			if (!BlockMetadata)
			{
				BlockMetadata = GridOfActors.Add(Block, NewObject<UBlockMetadata>(this));
			}

			if (BlockMetadata->ConstantActorsCount <= 0) // We keep the biome as well as all other objects
			{
				BlockMetadata->Biome = Delimiters[DelimiterIndex].Biome;
			}
			++BlockIndex;
		}
	}
}

void AMWorldGenerator::OnTickGenerateBlocks(TSet<FIntPoint> BlocksToGenerate)
{
	constexpr int BlocksPerFrame = 1;
	int Index = 0;
	for (auto It = BlocksToGenerate.CreateIterator(); It; ++It)
	{
		GenerateBlock(*It);
		It.RemoveCurrent();

		if (++Index >= BlocksPerFrame)
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick([this, BlocksToGenerate]{ OnTickGenerateBlocks(BlocksToGenerate); });
			return;;
		}
	}
}

FIntPoint AMWorldGenerator::GetPlayerGroundBlockIndex() const
{
	if (const auto pPlayerMetadata = ActorsMetadata.Find("Player"))
	{
		return GetGroundBlockIndex(pPlayerMetadata->Actor->GetActorLocation());
	}
	check(false);
	return {};
}

UBlockMetadata* AMWorldGenerator::FindOrAddBlock(FIntPoint Index)
{
	auto* BlockMetadata = GridOfActors.Get(Index);
	if (!BlockMetadata)
	{
		return GridOfActors.Add(Index, NewObject<UBlockMetadata>(this));
	}
	return BlockMetadata;
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

// Tick interval is set in the blueprint
void AMWorldGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckDynamicActorsBlocks();
}

AActor* AMWorldGenerator::SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation,
                                     const FActorSpawnParameters& SpawnParameters, bool bForceAboveGround, const FOnSpawnActorStarted& OnSpawnActorStarted)
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
			Actor->Rename(); // Guarantees unique name

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

	EnrollActorToGrid(Actor);

	if (const auto PickableActor = Cast<AMPickableActor>(Actor); PickableActor && ExperienceManager)
	{ // Enroll pickable actor to experience manager
		PickableActor->PickedUpCompletelyDelegate.AddDynamic(ExperienceManager, &UMExperienceManager::OnActorPickedUp);
	}

	return Actor;
}

void AMWorldGenerator::EnrollActorToGrid(AActor* Actor)
{
	if (!Actor)
		return;

	const auto GroundBlockIndex = GetGroundBlockIndex(Actor->GetActorLocation());

	// If there's an empty block, add it to the map
	auto BlockMetadata = GridOfActors.Get(GroundBlockIndex);
	if (!BlockMetadata)
	{
		BlockMetadata = GridOfActors.Add(GroundBlockIndex, NewObject<UBlockMetadata>(this));
	}

	// Determine whether the object is static or movable
	bool bDynamic = Actor->GetClass()->IsChildOf<APawn>();
	auto& ListToAdd = bDynamic ? BlockMetadata->DynamicActors : BlockMetadata->StaticActors;

	// We also store the mapping between the Name and metadata (actor's GroundBlock index, etc.)
	const FActorWorldMetadata Metadata{ListToAdd.Add(FName(Actor->GetName()), Actor), GroundBlockIndex};
	ActorsMetadata.Add(FName(Actor->GetName()), Metadata);

	// If was spawned on a disabled block, disable the actor (unless the actor has to be always enabled)
	if (const auto IsActiveCheckerComponent = Cast<UMIsActiveCheckerComponent>(Actor->GetComponentByClass(UMIsActiveCheckerComponent::StaticClass())))
	{
		if (IsActiveCheckerComponent->GetAlwaysEnabled() || IsActiveCheckerComponent->GetPreserveBlockConstancy())
		{
			BlockMetadata->ConstantActorsCount++; //TODO: Disable constancy when the object no longer on the block
		}
		else
		{
			if (!ActiveBlocksMap.Contains(GroundBlockIndex))
			{
				IsActiveCheckerComponent->DisableOwner();
			}
		}
	}
}

void AMWorldGenerator::RemoveActorFromGrid(AActor* Actor)
{
	//TODO: Accumulate and process such calls asynchronously. Now processed immediately.

	if (!IsValid(Actor))
	{
		check(false);
		return;
	}
	const auto BlockIndex = GetGroundBlockIndex(Actor->GetActorLocation());
	if (const auto BlockMetadata = GridOfActors.Get(BlockIndex))
	{
		if (const auto ActiveCheckerComp = Cast<UMIsActiveCheckerComponent>(Actor->GetComponentByClass(UMIsActiveCheckerComponent::StaticClass())))
		{
			if (ActiveCheckerComp->GetAlwaysEnabled())
			{
				BlockMetadata->ConstantActorsCount--;
			}
		}
		const auto ActorName = FName(Actor->GetName());
		if (Actor->GetClass()->IsChildOf(APawn::StaticClass()))
		{
			BlockMetadata->DynamicActors.Remove(ActorName);
		}
		else
		{
			BlockMetadata->StaticActors.Remove(ActorName);
		}

		ActorsMetadata.Remove(ActorName);

		Actor->Destroy();
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

	for (auto X = StartBlock.X; X <= FinishBlock.X; ++X)
	{
		for (auto Y = StartBlock.Y; Y <= FinishBlock.Y; ++Y)
		{
			if (const auto Block = GridOfActors.Get({X, Y}))
			{
				if (const auto& Actors = bDynamic ? Block->DynamicActors : Block->StaticActors;
					!Actors.IsEmpty())
				{
					for (const auto& [Name, Actor] : Actors)
					{
						if (const auto Metadata = ActorsMetadata.Find(Name); Actor)
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

void AMWorldGenerator::CleanArea(const FVector& Location, float Radius)
{
	const auto StartBlock = GetGroundBlockIndex(Location - FVector(Radius, Radius, 0.f));
	const auto FinishBlock = GetGroundBlockIndex(Location + FVector(Radius, Radius, 0.f));
	for (auto X = StartBlock.X; X <= FinishBlock.X; ++X)
	{
		for (auto Y = StartBlock.Y; Y <= FinishBlock.Y; ++Y)
		{
			if (const auto Block = GridOfActors.Get({X, Y}))
			{
				//TEMP SOLUTION
				for (auto It = Block->StaticActors.CreateIterator(); It; ++It)
				{
					if (Cast<AMGroundBlock>(It.Value()))
					{
						continue;
					}
					It->Value->Destroy();
					It.RemoveCurrent();
				}
			}
		}
	}
}

/** Finds the bounds of the default object for a blueprint. You have to mark components with AffectsDefaultBounds tag in order to affect bounds! */
FBoxSphereBounds AMWorldGenerator::GetDefaultBounds(UClass* IN_ActorClass, UObject* WorldContextObject)
{
	FBoxSphereBounds ActorBounds(ForceInitToZero);

	if (const auto pWorld = WorldContextObject->GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				if (const auto FoundBounds = pWorldGenerator->DefaultBoundsMap.Find(IN_ActorClass))
				{
					return *FoundBounds;
				}

				if (!IN_ActorClass)
				{
					check(false)
					return ActorBounds;
				}

				if (!pWorldGenerator->GridOfActors.Get(FIntPoint::ZeroValue))
				{
					auto Block = pWorldGenerator->GridOfActors.Add(FIntPoint::ZeroValue, NewObject<UBlockMetadata>(pWorldGenerator));
				}

				FActorSpawnParameters SpawnParameters;
				SpawnParameters.Name = FName("TestBounds_" + IN_ActorClass->GetName());
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (const auto Actor = pWorld->SpawnActor(IN_ActorClass, nullptr, nullptr, SpawnParameters))
				{
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
					pWorldGenerator->DefaultBoundsMap.Add(IN_ActorClass, ActorBounds);

					Actor->Destroy();
				}
			}
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
