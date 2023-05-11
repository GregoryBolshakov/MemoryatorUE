// Fill out your copyright notice in the Description page of Project Settings.

#include "MWorldGenerator.h"

#include "Math/UnrealMathUtility.h"
#include "MActor.h" 
#include "MCharacter.h"
#include "MGroundBlock.h"
#include "MAICrowdManager.h"
#include "MBlockGenerator.h"
#include "MCommunicationManager.h"
#include "MDropManager.h"
#include "MIsActiveCheckerComponent.h"
#include "MVillageGenerator.h"
#include "MWorldManager.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "NavMesh/NavMeshBoundsVolume.h"

AMWorldGenerator::AMWorldGenerator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ActiveZoneRadius(0)
	, DynamicActorsCheckInterval(0.5f)
	, DynamicActorsCheckTimer(0.f)
	, DropManager(nullptr)
	, CommunicationManager(nullptr)
	, BlockGenerator(nullptr)
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AMWorldGenerator::GenerateActiveZone()
{
	auto* pWorld = GetWorld();
	if (!pWorld)
		return;

	// Fill ActiveBlocksMap with the active blocks
	UpdateActiveZone();

	// Add player to the Grid
	const auto pPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	const auto PlayerBlockIndex = GetGroundBlockIndex(pPlayer->GetTransform().GetLocation());
	EnrollActorToGrid(pPlayer);

	// We add 1 to the radius on purpose. Generated area always has to be further then visible
	const auto BlocksInRadius = GetBlocksInRadius(PlayerBlockIndex.X, PlayerBlockIndex.Y, ActiveZoneRadius + 1);
	for (const auto BlockInRadius : BlocksInRadius)
	{ // Set the biomes in a separate pass first because we need to know each biome during block generation in order to disable/enable block transitions
		auto* BlockOfActors = GridOfActors.Contains(BlockInRadius) ?
					*GridOfActors.Find(BlockInRadius) :
					GridOfActors.Add(BlockInRadius, NewObject<UBlockOfActors>(this));
		BlockOfActors->Biome = EBiome::DarkWoods;
	}
	for (const auto BlockInRadius : BlocksInRadius)
	{
		GenerateBlock(BlockInRadius);
	}

	FActorSpawnParameters SpawnParameters;
	const auto VillageClass = ToSpawnComplexStructureClasses.Find("Village")->Get();
	const auto VillageGenerator = pWorld->SpawnActor<AMVillageGenerator>(VillageClass, FVector::Zero(), FRotator::ZeroRotator, SpawnParameters);
	VillageGenerator->Generate();
	UpdateNavigationMesh();
}

void AMWorldGenerator::GenerateBlock(const FIntPoint& BlockIndex, bool EraseDynamicObjects)
{
	const auto pWorld = GetWorld();
	if (!pWorld)
		return;

	// Get the block from grid or add if doesn't exist
	auto* BlockOfActors = GridOfActors.Contains(BlockIndex) ?
				*GridOfActors.Find(BlockIndex) :
				GridOfActors.Add(BlockIndex, NewObject<UBlockOfActors>(this));

	// Empty the block if already spawned
	for (auto It = BlockOfActors->StaticActors.CreateIterator(); It; ++It)
	{
		if (!IsValid(It->Value))
			continue;
		It->Value->Destroy();
		It.RemoveCurrent();
	}
	if (EraseDynamicObjects)
	{
		for (auto It = BlockOfActors->DynamicActors.CreateIterator(); It; ++It)
		{
			if (!IsValid(It->Value))
				continue;
			It->Value->Destroy();
			It.RemoveCurrent();
		}
	}

	BlockGenerator->Generate(BlockIndex, this, BlockOfActors->Biome);
}

void AMWorldGenerator::BeginPlay()
{
	Super::BeginPlay();
	UpdateActiveZone();

	// Create the Drop Manager
	DropManager = DropManagerBPClass ? NewObject<UMDropManager>(this, DropManagerBPClass, TEXT("DropManager")) : nullptr;
	check(DropManager);

	// Spawn the Communication Manager
	CommunicationManager = CommunicationManagerBPClass ? GetWorld()->SpawnActor<AMCommunicationManager>(CommunicationManagerBPClass) : nullptr;
	check(CommunicationManager);

	// Create the Block Generator
	BlockGenerator = BlockGeneratorBPClass ? NewObject<UMBlockGenerator>(this, BlockGeneratorBPClass, TEXT("BlockGenerator")) : nullptr;
	check(BlockGenerator);

	// We want to set the biome coloring since the first block change
	BlocksPassedSinceLastPerimeterColoring = BiomesPerimeterColoringRate;

	GenerateActiveZone();

	// Bind to the player-moves-to-another-block event 
	if (const auto pPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		if (const auto PlayerMetadata = ActorsMetadata.Find(FName(pPlayer->GetName())))
		{
			PlayerMetadata->OnBlockChangedDelegate.AddDynamic(this, &AMWorldGenerator::OnPlayerChangedBlock);
		}
	}
}

void AMWorldGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (const auto pPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		if (const auto PlayerMetadata = ActorsMetadata.Find(FName(pPlayer->GetName())))
		{
			PlayerMetadata->OnBlockChangedDelegate.RemoveDynamic(this, &AMWorldGenerator::OnPlayerChangedBlock);
		}
	}
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
		FActorWorldMetadata& Metadata;
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
		if (const auto Block = GridOfActors.Find(Index); Block && !(*Block)->DynamicActors.IsEmpty())
		{
			for (const auto& [Name, Data] : (*Block)->DynamicActors)
			{
				if (const auto ActorMetadata = ActorsMetadata.Find(Name))
				{
					if (!ActorMetadata->Actor) //TODO: Remove this temporary solution
					{
						// If the metadata has invalid Actor pointer, just delete this record
						const FTransition NewTransition{*ActorMetadata, **Block, nullptr};
						TransitionList.Add(Name, NewTransition);
					}
					else
					{
						if (const auto ActualBlockIndex = GetGroundBlockIndex(Data->GetTransform().GetLocation());
							ActorMetadata->GroundBlockIndex != ActualBlockIndex)
						{
							if (const auto NewBlock = GridOfActors.Find(ActualBlockIndex))
							{
								const FTransition NewTransition{*ActorMetadata, **Block, *NewBlock};
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
			Transition.NewBlock->DynamicActors.Add(Name, Transition.Metadata.Actor);

			// Even though the dynamic object is still enabled, it might have moved to the disabled block,
			// where all surrounding static objects are disabled.
			// Check the environment for validity if you bind to the delegate!
			Transition.Metadata.OnBlockChangedDelegate.Broadcast(Transition.Metadata.GroundBlockIndex);
		}
	}
}

void AMWorldGenerator::UpdateActiveZone()
{
	const auto PlayerLocation = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetTransform().GetLocation();
	const auto PlayerBlock = GetGroundBlockIndex(PlayerLocation);

	// Enable all the objects within PlayerActiveZone. ActiveBlocksMap is considered as from the previous check.
	TMap<FIntPoint, bool> ActiveBlocksMap_New;

	//TODO: Consider spreading the block logic over multiple ticks as done in OnTickGenerateBlocks()
	for (const auto Block : GetBlocksInRadius(PlayerBlock.X, PlayerBlock.Y, ActiveZoneRadius + 1)) // you can add +1 to the ActiveZoneRadius if you need to see how the perimeter is generated in PIE
	{
		ActiveBlocksMap_New.Add(Block, true);
		ActiveBlocksMap.Remove(Block);
		if (const auto GridBlock = GridOfActors.Find(Block);
			GridBlock && !(*GridBlock)->StaticActors.IsEmpty())
		{
			// Enable all the static Actors in the block
			for (const auto& [Index, Data] : (*GridBlock)->StaticActors)
			{
				//TODO: Add a function to check the Data for nullptr and if yes then remove the record from StaticActors and ActorsMetadata
				if (Data) // temporary
				{
					if (const auto IsActiveCheckerComponent = Data->FindComponentByClass<UMIsActiveCheckerComponent>())
					{
						IsActiveCheckerComponent->EnableOwner();
					}
				}
			}
			// Enable all dynamic Actors in the block
			for (const auto& [Index, Data] : (*GridBlock)->DynamicActors)
			{
				if (Data) // temporary
				{
					if (const auto IsActiveCheckerComponent = Data->FindComponentByClass<UMIsActiveCheckerComponent>())
					{
						IsActiveCheckerComponent->EnableOwner();
					}
				}
			}
		}
	}

	// Disable/Remove all the rest of objects that were in PlayerActiveZone in the previous check but no longer there.
	for (const auto& [BlockIndex, IsActive] : ActiveBlocksMap)
	{
		if (const auto GridBlock = GridOfActors.Find(BlockIndex))
		{
			for (const auto& [Index, Data] : (*GridBlock)->StaticActors)
			{
				if (Data) // temporary
				{
					if (const auto IsActiveCheckerComponent = Data->FindComponentByClass<UMIsActiveCheckerComponent>())
					{
						IsActiveCheckerComponent->DisableOwner();
					}
				}
			}
			for (const auto& [Index, Data] : (*GridBlock)->DynamicActors)
			{
				if (Data) // temporary
				{
					if (const auto IsActiveCheckerComponent = Data->FindComponentByClass<UMIsActiveCheckerComponent>())
					{
						IsActiveCheckerComponent->DisableOwner();
					}
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
	const auto pWorld = GetWorld();
	if (!pWorld)
	{
		check(false);
		return;
	}

	const auto PlayerLocation = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetTransform().GetLocation();
	if (const auto NavMeshVolume = Cast<ANavMeshBoundsVolume>(UGameplayStatics::GetActorOfClass(pWorld, ANavMeshBoundsVolume::StaticClass())))
	{
		NavMeshVolume->SetActorLocation(PlayerLocation);

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
void AMWorldGenerator::OnPlayerChangedBlock(const FIntPoint& NewBlock)
{
	auto pWorld = GetWorld();
	if (!pWorld)
		return;

	UpdateActiveZone(); // heavy call, we put everything else to the next tick

	pWorld->GetTimerManager().SetTimerForNextTick([this, pWorld, NewBlock]
	{
		UpdateNavigationMesh(); // heavy call, we put everything else to the next tick

		pWorld->GetTimerManager().SetTimerForNextTick([this, pWorld, NewBlock]
		{ // Generate the perimeter outside the active zone
			auto BlocksInRadius = GetBlocksOnPerimeter(NewBlock.X, NewBlock.Y, ActiveZoneRadius + 1);

			SetBiomesForBlocks(NewBlock, BlocksInRadius);

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
		});
	});
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

			auto* BlockOfActors = GridOfActors.Contains(Block) ?
				*GridOfActors.Find(Block) :
				GridOfActors.Add(Block, NewObject<UBlockOfActors>(this));

			BlockOfActors->Biome = Delimiters[DelimiterIndex].Biome;
			++BlockIndex;
		}
	}
}

int BlocksPerFrame = 1;
void AMWorldGenerator::OnTickGenerateBlocks(TSet<FIntPoint> BlocksToGenerate)
{
	int Index = 0;
	for (auto It = BlocksToGenerate.CreateIterator(); It; ++It)
	{
		GenerateBlock(*It);
		It.RemoveCurrent();

		if (++Index >= BlocksPerFrame)
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick([this, BlocksToGenerate]{ OnTickGenerateBlocks(BlocksToGenerate); });
			break;
		}
	}
}

FVector AMWorldGenerator::GetGroundBlockSize()
{
	if (const auto ToSpawnGroundBlock = ToSpawnActorClasses.Find(FName("GroundBlock")); GetWorld())
	{
		const auto GroundBlockBounds = GetDefaultBounds(ToSpawnGroundBlock->Get(), GetWorld());
		return GroundBlockBounds.BoxExtent * 2.f;
	}
	check(false);
	return FVector::ZeroVector;
}

FIntPoint AMWorldGenerator::GetGroundBlockIndex(FVector Position)
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
	return FVector(BlockIndex) * GroundBlockSize;
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

void AMWorldGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DynamicActorsCheckTimer += DeltaSeconds;
	if (DynamicActorsCheckTimer >= DynamicActorsCheckInterval)
	{
		DynamicActorsCheckTimer = 0.f;
		CheckDynamicActorsBlocks();
	}
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

	auto* Actor = pWorld->SpawnActorDeferred<AActor>(Class, ActorTransform, nullptr, nullptr, SpawnParameters.SpawnCollisionHandlingOverride);

	OnSpawnActorStarted.Broadcast(Actor);

	UGameplayStatics::FinishSpawningActor(Actor, ActorTransform);

	if (!Actor)
	{
		check(false);
		return nullptr;
	}

	EnrollActorToGrid(Actor);

	return Actor;
}

void AMWorldGenerator::EnrollActorToGrid(AActor* Actor, bool bMakeBlockConstant)
{
	if (!Actor)
		return;

	const auto GroundBlockIndex = GetGroundBlockIndex(Actor->GetActorLocation());

	// If there's an empty block, add it to the map
	UBlockOfActors* BlockOfActors;
	if (!GridOfActors.Contains(GroundBlockIndex))
	{
		BlockOfActors = GridOfActors.Add(GroundBlockIndex, NewObject<UBlockOfActors>(this));
	}
	else
	{
		BlockOfActors = *GridOfActors.Find(GroundBlockIndex);
	}

	if (bMakeBlockConstant)
	{
		BlockOfActors->IsConstant = true;
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
		if (IsActiveCheckerComponent->GetAlwaysEnabled())
		{
			BlockOfActors->IsConstant = true;
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
			if (const auto Block = GridOfActors.Find({X, Y}))
			{
				if (const auto& Actors = bDynamic ? (*Block)->DynamicActors : (*Block)->StaticActors;
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
			if (const auto Block = GridOfActors.Find({X, Y}))
			{
				//TEMP SOLUTION
				for (auto It = (*Block)->StaticActors.CreateIterator(); It; ++It)
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
TMap<UClass*, FBoxSphereBounds> AMWorldGenerator::DefaultBoundsMap;
/** Finds the bounds of the default object for a blueprint. You have to mark components with AffectsDefaultBounds tag in order to affect bounds! */
FBoxSphereBounds AMWorldGenerator::GetDefaultBounds(UClass* IN_ActorClass, UObject* WorldContextObject)
{
	if (const auto FoundBounds = DefaultBoundsMap.Find(IN_ActorClass))
	{
		return *FoundBounds;
	}

	FBoxSphereBounds ActorBounds(ForceInitToZero);

	if (!IN_ActorClass)
		return ActorBounds;

	if (const auto pWorld = WorldContextObject->GetWorld())
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (const auto Actor = pWorld->SpawnActor(IN_ActorClass, {}, {}, SpawnParameters))
		{
			// Calculate the Actor bounds by accumulating the bounds of its components
			FBox ActorBox(EForceInit::ForceInitToZero);
			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component); PrimitiveComponent &&
					PrimitiveComponent->ComponentHasTag("AffectsDefaultBounds")) //TODO: Come up with getting collision enabled state of not fully initialized component
				{
					FTransform ComponentTransform = PrimitiveComponent->GetComponentTransform();
					FBoxSphereBounds ComponentBounds = PrimitiveComponent->CalcBounds(ComponentTransform);
					ActorBox += ComponentBounds.GetBox();
				}
			}

			ActorBounds.Origin = ActorBox.GetCenter();
			ActorBounds.BoxExtent = ActorBox.GetExtent();
			ActorBounds.SphereRadius = ActorBox.GetExtent().Size2D();
			DefaultBoundsMap.Add(IN_ActorClass, ActorBounds);

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
