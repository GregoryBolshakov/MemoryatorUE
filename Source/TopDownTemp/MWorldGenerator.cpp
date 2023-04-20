// Fill out your copyright notice in the Description page of Project Settings.

#include "MWorldGenerator.h"

#include "Math/UnrealMathUtility.h"
#include "MActor.h" 
#include "MCharacter.h"
#include "MGroundBlock.h"
#include "MAICrowdManager.h"
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
	auto pPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	const auto PlayerBlockIndex = GetGroundBlockIndex(pPlayer->GetTransform().GetLocation());
	GridOfActors.Add(PlayerBlockIndex, NewObject<UBlockOfActors>(this))->DynamicActors.Emplace(pPlayer->GetName(), pPlayer);
	ActorsMetadata.Add(FName(pPlayer->GetName()), {pPlayer, PlayerBlockIndex});

	// Get ground block class and bounds
	const auto ToSpawnGroundBlock = ToSpawnActorClasses.Find(FName("GroundBlock"));
	const auto GroundBlockBounds = GetDefaultBounds(ToSpawnGroundBlock->Get(), pWorld);
	if (GroundBlockBounds.BoxExtent == FVector::ZeroVector)
		return;

	// We add 1 to the radius on purpose. Generated area always has to be further then visible
	for (const auto BlockInRadius : GetBlocksInRadius(PlayerBlockIndex.X, PlayerBlockIndex.Y, ActiveZoneRadius + 1))
	{
		GenerateBlock(BlockInRadius);
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = "VillageGenerator_1";
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

	// Get the ground block size
	const auto ToSpawnGroundBlock = ToSpawnActorClasses.Find(FName("GroundBlock"));
	if (!ToSpawnGroundBlock)
		return;

	const auto GroundBlockBounds = GetDefaultBounds(ToSpawnGroundBlock->Get(), pWorld);
	if (GroundBlockBounds.BoxExtent == FVector::ZeroVector)
		return;
	const auto GroundBlockSize = GroundBlockBounds.BoxExtent * 2.f;

	// Empty the block if already spawned
	if (const auto BlockOfActors = GridOfActors.Find(BlockIndex))
	{
		for (auto It = (*BlockOfActors)->StaticActors.CreateIterator(); It; ++It)
		{
			if (!IsValid(It->Value))
				continue;
			It->Value->Destroy();
			It.RemoveCurrent();
		}
		if (EraseDynamicObjects)
		{
			for (auto It = (*BlockOfActors)->DynamicActors.CreateIterator(); It; ++It)
			{
				if (!IsValid(It->Value))
					continue;
				It->Value->Destroy();
				It.RemoveCurrent();
			}
		}
	}

	const FVector Location(GroundBlockSize.X * BlockIndex.X, GroundBlockSize.Y * BlockIndex.Y, 0);

	FActorSpawnParameters BlockSpawnParameters;
	BlockSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* GroundBlock = SpawnActor<AMGroundBlock>(ToSpawnGroundBlock->Get(), Location, FRotator::ZeroRotator, BlockSpawnParameters);

	FActorSpawnParameters EmptySpawnParameters;
	auto* Tree = SpawnActor<AMActor>(*ToSpawnActorClasses.Find(FName("Tree")), Location, FRotator::ZeroRotator, EmptySpawnParameters);
}

void AMWorldGenerator::BeginPlay()
{
	Super::BeginPlay();
	UpdateActiveZone();

	// Bind to the player-moves-to-another-block event 
	if (const auto pPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		if (const auto PlayerMetadata = ActorsMetadata.Find(FName(pPlayer->GetName())))
		{
			PlayerMetadata->OnBlockChangedDelegate.AddDynamic(this, &AMWorldGenerator::OnPlayerChangedBlock);
		}
	}

	// Create the Drop Manager
	DropManager = DropManagerBPClass ? NewObject<UMDropManager>(this, DropManagerBPClass, TEXT("DropManager")) : nullptr;
	check(DropManager);
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
	for (const auto Block : GetBlocksInRadius(PlayerBlock.X, PlayerBlock.Y, ActiveZoneRadius))
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

void AMWorldGenerator::SetBiomesForBlocks(const FIntPoint& CenterBlock, TSet<FIntPoint>& BlocksToGenerate)
{
	// Sort the blocks in ascending order of the polar angle
	BlocksToGenerate.Sort([&CenterBlock](const FIntPoint& BlockA, const FIntPoint& BlockB) {
		return GetAngle(CenterBlock, BlockA) < GetAngle(CenterBlock, BlockB);
	});

	
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

FIntPoint AMWorldGenerator::GetGroundBlockIndex(FVector Position)
{
	if (const auto ToSpawnGroundBlock = ToSpawnActorClasses.Find(FName("GroundBlock")); GetWorld())
	{
		const auto GroundBlockBounds = GetDefaultBounds(ToSpawnGroundBlock->Get(), GetWorld());
		const auto GroundBlockSize = GroundBlockBounds.BoxExtent * 2.f;
		return FIntPoint(FMath::FloorToInt(Position.X/GroundBlockSize.X), FMath::FloorToInt(Position.Y/GroundBlockSize.Y));
	}
	check(false);
	return {0, 0};
}

FVector AMWorldGenerator::GetGroundBlockLocation(FIntPoint BlockIndex)
{
	if (const auto ToSpawnGroundBlock = ToSpawnActorClasses.Find(FName("GroundBlock")); GetWorld())
	{
		const auto GroundBlockBounds = GetDefaultBounds(ToSpawnGroundBlock->Get(), GetWorld());
		const auto GroundBlockSize = GroundBlockBounds.BoxExtent * 2.f;
		return { FVector(BlockIndex) * GroundBlockSize };
	}
	check(false);
	return FVector::ZeroVector;
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
			if (Distance <= static_cast<float>(RadiusInBlocks) && Distance > static_cast<float>(RadiusInBlocks - 1)) {
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
                                     const FActorSpawnParameters& SpawnParameters, bool bForceAboveGround)
{
	const auto pWorld = GetWorld();
	if (!pWorld || !Class)
	{
		check(false);
		return nullptr;
	}

	AActor* Actor;

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
		Actor = pWorld->SpawnActor(Class, &LocationAboveGround, &Rotation, SpawnParameters);
	}
	else
	{
		Actor = pWorld->SpawnActor(Class, &Location, &Rotation, SpawnParameters);
	}

	if (!Actor)
		return nullptr;

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
		SpawnParameters.Name = MakeUniqueObjectName(WorldContextObject, IN_ActorClass);
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (const auto Actor = pWorld->SpawnActor(IN_ActorClass, {}, {}, SpawnParameters))
		{
			FVector Origin, BoxExtent;
			Actor->InitializeComponents();
			Actor->PostInitializeComponents();

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

AActor* AMWorldGenerator::SpawnActorInRadius(UClass* Class, const float ToSpawnRadius, const float ToSpawnHeight)
{
	const auto pWorld = GetWorld();
	if (!pWorld)
		return nullptr;

	const auto pPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!pPlayer)
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

		FVector SpawnPosition = pPlayer->GetTransform().GetLocation() + SpawnPositionOffset;
		SpawnPosition.Z = ToSpawnHeight;

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = MakeUniqueObjectName(GetWorld(), Class);
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
		auto Actor = SpawnActor<AActor>(Class, SpawnPosition, {}, SpawnParameters, true);
		if (Actor)
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
	return SpawnActorInRadius(Class, ToSpawnRadius + BoundsRadius * 2.f, ToSpawnHeight);
}
