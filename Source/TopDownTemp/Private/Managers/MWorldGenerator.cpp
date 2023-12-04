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

AMWorldGenerator::AMWorldGenerator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ActiveZoneRadius(0)
	, DropManager(nullptr)
	, ReputationManager(nullptr)
	, ExperienceManager(nullptr)
	, SaveManager(nullptr)
	, CommunicationManager(nullptr)
	, BlockGenerator(nullptr)
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}

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

	UpdateActiveZone(); // temp solution to avoid disabling all subsequently generated actors due to empty ActiveBlocksMap

	const auto PlayerBlockIndex = GetGroundBlockIndex(pPlayer->GetTransform().GetLocation());

	if (GridOfActors.Num() == 1 || ActorsMetadata.Num() == 1) // Empty world check
	{
		check(GridOfActors.Num() == 1 && ActorsMetadata.Num() == 1); // Error occured while loading save

		// We add 1 to the radius on purpose. Generated area always has to be further then visible
		const auto BlocksInRadius = GetBlocksInRadius(PlayerBlockIndex.X, PlayerBlockIndex.Y, ActiveZoneRadius + 1);
		for (const auto BlockInRadius : BlocksInRadius)
		{ // Set the biomes in a separate pass first because we need to know each biome during block generation in order to disable/enable block transitions
			auto* BlockOfActors = GridOfActors.Get(BlockInRadius);
			if (!BlockOfActors)
			{
				BlockOfActors = GridOfActors.Add(BlockInRadius, NewObject<UBlockOfActors>(this));
			}
			BlockOfActors->Biome = BiomeForInitialGeneration;
		}
		for (const auto BlockInRadius : BlocksInRadius)
		{
			GenerateBlock(BlockInRadius);
		}

		/*const auto VillageClass = ToSpawnComplexStructureClasses.Find("Village")->Get();
		const auto VillageGenerator = pWorld->SpawnActor<AMVillageGenerator>(VillageClass, FVector::Zero(), FRotator::ZeroRotator);
		VillageGenerator->Generate();
		UpdateNavigationMesh();*/

		/*EmptyBlock({PlayerBlockIndex.X, PlayerBlockIndex.Y}, true);
		BlockGenerator->SpawnActors({PlayerBlockIndex.X, PlayerBlockIndex.Y}, this, EBiome::BirchGrove, "TestBlock");*/
	}
}

UBlockOfActors* AMWorldGenerator::EmptyBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects, bool IgnoreConstancy)
{
	const auto pWorld = GetWorld();
	if (!pWorld)
		return nullptr;

	// Get the block from grid or add if doesn't exist
	auto* BlockOfActors = GridOfActors.Get(BlockIndex);
	if (!BlockOfActors)
	{
		BlockOfActors = GridOfActors.Add(BlockIndex, NewObject<UBlockOfActors>(this));
	}

	if (BlockOfActors->ConstantActorsCount > 0 && !IgnoreConstancy)
		return BlockOfActors;

	// Empty the block if already spawned
	for (auto It = BlockOfActors->StaticActors.CreateIterator(); It; ++It)
	{
		if (It->Value)
		{
			RemoveActorFromGrid(It->Value);
		}
	}
	check(BlockOfActors->StaticActors.IsEmpty());

	if (!KeepDynamicObjects)
	{
		for (auto It = BlockOfActors->DynamicActors.CreateIterator(); It; ++It)
		{
			if (It->Value)
			{
				RemoveActorFromGrid(It->Value);
			}
		}
		check(BlockOfActors->DynamicActors.IsEmpty());
	}

	return BlockOfActors;
}

UBlockOfActors* AMWorldGenerator::GenerateBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects)
{
	const auto Block = EmptyBlock(BlockIndex, KeepDynamicObjects);
	BlockGenerator->SpawnActors(BlockIndex, this, Block->Biome);
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
		UBlockOfActors& OldBlock;
		UPROPERTY()
		UBlockOfActors* NewBlock;
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
		if (const auto GridBlock = GridOfActors.Get(Block);
			GridBlock && !GridBlock->StaticActors.IsEmpty())
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
		OldBlockIndex.X += FMath::Sign(IN_NewBlockIndex.X - OldBlockIndex.X);
		OldBlockIndex.Y += FMath::Sign(IN_NewBlockIndex.Y - OldBlockIndex.Y);
		TravelledDequeue.Add(OldBlockIndex);
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

	//UpdateActiveZone(); // heavy call, we put everything else to the next tick

	//pWorld->GetTimerManager().SetTimerForNextTick([this, pWorld, NewBlock]
	//{
	//UpdateNavigationMesh(); // heavy call, we put everything else to the next tick

	//pWorld->GetTimerManager().SetTimerForNextTick([this, pWorld, NewBlock]
	//{ // Generate the perimeter outside the active zone
	auto BlocksInRadius = GetBlocksOnPerimeter(CenterBlock.X, CenterBlock.Y, ActiveZoneRadius + 1);

	SetBiomesForBlocks(CenterBlock, BlocksInRadius);

	auto TopLeftScreenPointInWorld = RaycastScreenPoint(pWorld, EScreenPoint::TopLeft);
	auto TopRighScreenPointInWorld = RaycastScreenPoint(pWorld, EScreenPoint::TopRight);
	BlocksInRadius.Sort([this, &TopLeftScreenPointInWorld, &TopRighScreenPointInWorld](const FIntPoint& BlockA, const FIntPoint& BlockB)
	{ // Sort blocks so that the closest to the screen corners would be the first
		const auto LocationA = GetGroundBlockLocation(BlockA);
		const auto LocationB = GetGroundBlockLocation(BlockB);

		const auto MinDistanceToScreenEdgesA = FMath::Min(FVector::Distance(LocationA, TopLeftScreenPointInWorld), FVector::Distance(LocationA, TopRighScreenPointInWorld));
		const auto MinDistanceToScreenEdgesB = FMath::Min(FVector::Distance(LocationB, TopLeftScreenPointInWorld), FVector::Distance(LocationB, TopRighScreenPointInWorld));

		return MinDistanceToScreenEdgesA <= MinDistanceToScreenEdgesB;
	});

	// We'll spread heavy GenerateBlock calls over the next few ticks
	OnTickGenerateBlocks(BlocksInRadius);
	//});
	//});
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

		// Old implementation. Deprecated due to a lot of random. However is very customizable.
		/*if (BiomesNumberInCurrentColoring == 1)
		{
			Delimiters = { { BlocksToGenerate.Num() - 1, static_cast<EBiome>(FMath::RandRange(0, StaticEnum<EBiome>()->NumEnums() - 1)) } };
		}
		else
		{
			for (int i = 0; i < BiomesNumberInCurrentColoring - 1; ++i)
			{
				const int PreviousPosition = i > 0 ? Delimiters[i-1].BlockPosition : 0;
				Delimiters.Add({
					FMath::RandRange(PreviousPosition + 1, BlocksToGenerate.Num() - (BiomesNumberInCurrentColoring - i)), // random block index (including space for the rest biomes)
					static_cast<EBiome>(FMath::RandRange(0, StaticEnum<EBiome>()->NumEnums() - 1)) // random biome
				});
			}

			Delimiters.Add({
				BlocksToGenerate.Num() - 1, // the last sub-array ends on the last block
				static_cast<EBiome>(FMath::RandRange(0, StaticEnum<EBiome>()->NumEnums() - 1)) // random biome
			});
		}*/
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

			auto* BlockOfActors = GridOfActors.Get(Block);
			if (!BlockOfActors)
			{
				BlockOfActors = GridOfActors.Add(Block, NewObject<UBlockOfActors>(this));
			}

			if (BlockOfActors->ConstantActorsCount <= 0) // We keep the biome as well as all other objects
			{
				BlockOfActors->Biome = Delimiters[DelimiterIndex].Biome;
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

UBlockOfActors* AMWorldGenerator::FindOrAddBlock(FIntPoint Index)
{
	auto* BlockOfActors = GridOfActors.Get(Index);
	if (!BlockOfActors)
	{
		return GridOfActors.Add(Index, NewObject<UBlockOfActors>(this));
	}
	return BlockOfActors;
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

TSet<FIntPoint> AMWorldGenerator::GetBlocksOnPerimeter(int BlockX, int BlockY, int RadiusInBlocks)
{
	TSet<FIntPoint> Blocks;

	for (float X = BlockX - RadiusInBlocks; X <= BlockX + RadiusInBlocks; ++X)
	{
		for (float Y = BlockY - RadiusInBlocks; Y <= BlockY + RadiusInBlocks; ++Y)
		{
			const auto Distance = sqrt(pow(X - BlockX, 2) + pow(Y - BlockY, 2));
			if (Distance <= static_cast<float>(RadiusInBlocks) && Distance >= static_cast<float>(RadiusInBlocks - 1)) {
				Blocks.Add({ static_cast<int>(X), static_cast<int>(Y) });
			}
		}
	}

	return Blocks;
}

TSet<FIntPoint> AMWorldGenerator::GetBlocksInRadius(int BlockX, int BlockY, int RadiusInBlocks) const
{
	TSet<FIntPoint> Blocks;

	for (float X = BlockX - RadiusInBlocks; X <= BlockX + RadiusInBlocks; ++X)
	{
		for (float Y = BlockY - RadiusInBlocks; Y <= BlockY + RadiusInBlocks; ++Y)
		{
			if (sqrt(pow(X - BlockX, 2) + pow(Y - BlockY, 2)) <= static_cast<float>(RadiusInBlocks)) {
				Blocks.Add({ static_cast<int>(X), static_cast<int>(Y) });
			}
		}
	}

	return Blocks;
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
	auto BlockOfActors = GridOfActors.Get(GroundBlockIndex);
	if (!BlockOfActors)
	{
		BlockOfActors = GridOfActors.Add(GroundBlockIndex, NewObject<UBlockOfActors>(this));
	}

	// Determine whether the object is static or movable
	bool bDynamic = Actor->GetClass()->IsChildOf<APawn>();
	auto& ListToAdd = bDynamic ? BlockOfActors->DynamicActors : BlockOfActors->StaticActors;

	// We also store the mapping between the Name and metadata (actor's GroundBlock index, etc.)
	const FActorWorldMetadata Metadata{ListToAdd.Add(FName(Actor->GetName()), Actor), GroundBlockIndex};
	ActorsMetadata.Add(FName(Actor->GetName()), Metadata);

	// If was spawned on a disabled block, disable the actor (unless the actor has to be always enabled)
	if (const auto IsActiveCheckerComponent = Cast<UMIsActiveCheckerComponent>(Actor->GetComponentByClass(UMIsActiveCheckerComponent::StaticClass())))
	{
		if (IsActiveCheckerComponent->GetAlwaysEnabled() || IsActiveCheckerComponent->GetPreserveBlockConstancy())
		{
			BlockOfActors->ConstantActorsCount++; //TODO: Disable constancy when the object no longer on the block
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
	if (const auto BlockOfActors = GridOfActors.Get(BlockIndex))
	{
		if (const auto ActiveCheckerComp = Cast<UMIsActiveCheckerComponent>(Actor->GetComponentByClass(UMIsActiveCheckerComponent::StaticClass())))
		{
			if (ActiveCheckerComp->GetAlwaysEnabled())
			{
				BlockOfActors->ConstantActorsCount--;
			}
		}
		const auto ActorName = FName(Actor->GetName());
		if (Actor->GetClass()->IsChildOf(APawn::StaticClass()))
		{
			BlockOfActors->DynamicActors.Remove(ActorName);
		}
		else
		{
			BlockOfActors->StaticActors.Remove(ActorName);
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
					auto Block = pWorldGenerator->GridOfActors.Add(FIntPoint::ZeroValue, NewObject<UBlockOfActors>(pWorldGenerator));
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
