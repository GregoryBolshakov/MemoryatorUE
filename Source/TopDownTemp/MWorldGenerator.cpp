// Fill out your copyright notice in the Description page of Project Settings.

#include "MWorldGenerator.h"

#include "Math/UnrealMathUtility.h"
#include "MActor.h" 
#include "MCharacter.h"
#include "MGroundBlock.h"
#include "MMemoryator.h"
#include "MAICrowdManager.h"
#include "MIsActiveCheckerComponent.h"
#include "MVillageGenerator.h"
#include "MWorldManager.h"
#include "NavigationSystem.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Engine/SCS_Node.h"

AMWorldGenerator::AMWorldGenerator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DynamicActorsCheckInterval(0.5f)
	, DynamicActorsCheckTimer(0.f)
{
	PlayerActiveZone = CreateDefaultSubobject<UBoxComponent>(TEXT("Player Active Zone"));
	PlayerActiveZone->SetBoxExtent(FVector(500.0f, 500.0f, 500.0f));
	PlayerActiveZone->SetGenerateOverlapEvents(false);
	PlayerActiveZone->PrimaryComponentTick.bStartWithTickEnabled = false;
	PlayerActiveZone->PrimaryComponentTick.bCanEverTick = false;
	SetRootComponent(PlayerActiveZone);

	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AMWorldGenerator::GenerateWorld()
{
	//TODO: Remove it from here
	auto pPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	const auto Block = GetGroundBlockIndex(pPlayer->GetTransform().GetLocation());
	GridOfActors.Add(Block, {}).DynamicActors.Emplace(pPlayer->GetName(), pPlayer);
	ActorsMetadata.Add(FName(pPlayer->GetName()), {pPlayer, Block});

	auto* pWorld = GetWorld();
	if (!pWorld)
	{
		check(false);
		return;
	}

	const auto ToSpawnGroundBlock = ToSpawnActorClasses.Find(FName("GroundBlock"));
	if (!ToSpawnGroundBlock)
	{
		check(false);
		return;
	}
	const auto GroundBlockBounds = GetDefaultBounds(ToSpawnGroundBlock->Get(), pWorld);
	if (GroundBlockBounds.BoxExtent == FVector::ZeroVector)
		return;
	const auto GroundBlockSize = GroundBlockBounds.BoxExtent * 2.f;

	// Adjust the world size so that it is divisible by two block sizes
	WorldSize.X /= 2 * GroundBlockSize.X;
	WorldSize.X *= 2 * GroundBlockSize.X;
	WorldSize.Y /= 2 * GroundBlockSize.Y;
	WorldSize.Y *= 2 * GroundBlockSize.Y;

	for (int x = -WorldSize.X / 2; x < WorldSize.X / 2; x += GroundBlockSize.X)
	{
		for (int y = -WorldSize.Y / 2; y < WorldSize.Y / 2; y += GroundBlockSize.Y)
		{
			FVector Location(x, y, 0);
			FActorSpawnParameters EmptySpawnParameters{};
			auto* GroundBlock = SpawnActor<AMGroundBlock>(ToSpawnGroundBlock->Get(), Location, FRotator::ZeroRotator, EmptySpawnParameters);

			int TreeSpawnRate = FMath::RandRange(1, 5);
			//if (TreeSpawnRate == 1)
			{
				const auto TreeDefault = GetDefault<AMActor>(*ToSpawnActorClasses.Find(FName("Tree")));
				FVector TreeLocation(x, y, 0);
				EmptySpawnParameters = {};
				auto* Tree = SpawnActor<AMActor>(*ToSpawnActorClasses.Find(FName("Tree")), TreeLocation, FRotator::ZeroRotator, EmptySpawnParameters);
			}
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = "VillageGenerator_1";
	const auto VillageClass = ToSpawnComplexStructureClasses.Find("Village")->Get();
	const auto VillageGenerator = pWorld->SpawnActor<AMVillageGenerator>(VillageClass, FVector::Zero(), FRotator::ZeroRotator, SpawnParameters);
	VillageGenerator->Generate();
	UpdateNavigationMesh();
}

void AMWorldGenerator::BeginPlay()
{
	Super::BeginPlay();
	UpdateActiveZone();

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
	struct FTransition
	{
		FActorWorldMetadata& Metadata;
		FBlockOfActors& OldBlock;
		FBlockOfActors* NewBlock;
	};
	TMap<FName, FTransition> TransitionList;

	for (const auto [Index, Bool] : ActiveBlocksMap)
	{
		if (!Bool)
		{
			check(false)
			continue;
		}
		if (const auto Block = GridOfActors.Find(Index); Block && !Block->DynamicActors.IsEmpty())
		{
			for (const auto& [Name, Data] : Block->DynamicActors)
			{
				if (const auto ActorMetadata = ActorsMetadata.Find(Name))
				{
					if (!ActorMetadata->Actor) //TODO: Remove this temporary solution
					{
						// If the metadata has invalid Actor pointer, just delete this record
						const FTransition NewTransition{*ActorMetadata, *Block, nullptr};
						TransitionList.Add(Name, NewTransition);
					}
					else
					{
						if (const auto ActualBlockIndex = GetGroundBlockIndex(Data->GetTransform().GetLocation());
							ActorMetadata->GroundBlockIndex != ActualBlockIndex)
						{
							if (const auto NewBlock = GridOfActors.Find(ActualBlockIndex))
							{
								const FTransition NewTransition{*ActorMetadata, *Block, NewBlock};
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
			Transition.Metadata.OnBlockChangedDelegate.Broadcast();
		}
	}
}

void AMWorldGenerator::UpdateActiveZone()
{
	const auto PlayerLocation = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetTransform().GetLocation();
	//TODO: Think how to fit the Active zone to the screen size
	PlayerActiveZone->SetWorldLocation(PlayerLocation);

	const FTransform BoxTransform = PlayerActiveZone->GetComponentToWorld();
	const FVector BoxExtent = PlayerActiveZone->GetScaledBoxExtent();

	const auto StartBlock = GetGroundBlockIndex(BoxTransform.GetLocation() - BoxExtent);
	const auto FinishBlock = GetGroundBlockIndex(BoxTransform.GetLocation() + BoxExtent);

	// Enable all the objects within PlayerActiveZone. ActiveBlocksMap is considered as from the previous check.
	TMap<FIntPoint, bool> ActiveBlocksMap_New;
	for (auto X = StartBlock.X; X <= FinishBlock.X; ++X)
	{
		for (auto Y = StartBlock.Y; Y <= FinishBlock.Y; ++Y)
		{
			ActiveBlocksMap_New.Add(FIntPoint(X, Y), true);
			ActiveBlocksMap.Remove(FIntPoint(X, Y));
			if (const auto Block = GridOfActors.Find(FIntPoint(X, Y));
				Block && !Block->StaticActors.IsEmpty())
			{
				// Enable all the static Actors in the block
				for (const auto& [Index, Data] : Block->StaticActors)
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
				for (const auto& [Index, Data] : Block->DynamicActors)
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
	}

	// Disable all the rest of objects that were in PlayerActiveZone in the previous check but no longer there.
	for (const auto& [BlockIndex, IsActive] : ActiveBlocksMap)
	{
		if (const auto Block = GridOfActors.Find(BlockIndex))
		{
			for (const auto& [Index, Data] : Block->StaticActors)
			{
				if (Data) // temporary
				{
					if (const auto IsActiveCheckerComponent = Data->FindComponentByClass<UMIsActiveCheckerComponent>())
					{
						IsActiveCheckerComponent->DisableOwner();
					}
				}
			}
			for (const auto& [Index, Data] : Block->DynamicActors)
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

void AMWorldGenerator::OnPlayerChangedBlock()
{
	UpdateActiveZone();
	UpdateNavigationMesh();
}

FIntPoint AMWorldGenerator::GetGroundBlockIndex(FVector Position)
{
	if (const auto ToSpawnGroundBlock = ToSpawnActorClasses.Find(FName("GroundBlock")); GetWorld())
	{
		const auto GroundBlockBounds = GetDefaultBounds(ToSpawnGroundBlock->Get(), GetWorld());
		const auto GroundBlockSize = GroundBlockBounds.BoxExtent * 2.f;
		return FIntPoint(FMath::FloorToInt(Position.X/GroundBlockSize.X), FMath::FloorToInt(Position.Y/GroundBlockSize.Y));
	}

	return {0, 0};
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

	const auto GroundBlockIndex = GetGroundBlockIndex(Location);

	// If there's an empty block, add it to the map
	auto BlockOfActors = GridOfActors.Find(GroundBlockIndex);
	if (!BlockOfActors)
	{
		BlockOfActors = &GridOfActors.Add(GroundBlockIndex, {});
	}

	// Determine whether the object is static or movable
	bool bDynamic = Class->IsChildOf<APawn>();
	auto& ListToAdd = bDynamic ? BlockOfActors->DynamicActors : BlockOfActors->StaticActors;

	//TODO: If this check is of no use, should be removed
	if (const auto ExistingActor = ListToAdd.Find(SpawnParameters.Name))
	{
		UE_LOG(LogWorldManager, Warning, TEXT("Trying to spawn already existing actor"));
		return *ExistingActor;
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

	// We also store the mapping between the Name and metadata (actor's GroundBlock index, etc.)
	const FActorWorldMetadata Metadata{ListToAdd.Add(SpawnParameters.Name, Actor), GroundBlockIndex};
	ActorsMetadata.Add(SpawnParameters.Name, Metadata);

	return Metadata.Actor;
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
			if (const auto Block = GridOfActors.Find({X, Y}))
			{
				//TEMP SOLUTION
				for (auto It = Block->StaticActors.CreateIterator(); It; ++It)
				{
					if (Cast<AMGroundBlock>(It.Value()))
					{
						continue;
					}
					It.RemoveCurrent();
				}
			}
		}
	}
}
TMap<UClass*, FBoxSphereBounds> AMWorldGenerator::DefaultBoundsMap;
/** Finds the bounds of the default object for a blueprint */
FBoxSphereBounds AMWorldGenerator::GetDefaultBounds(UClass* IN_ActorClass, UObject* WorldContextObject)
{
	if (const auto FoundBounds = DefaultBoundsMap.Find(IN_ActorClass))
	{
		return *FoundBounds;
	}

	FBoxSphereBounds ActorBounds(ForceInitToZero);

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

	const auto DefaultBounds = GetDefaultBounds(Class, pWorld);
	const auto BoundsRadius = FMath::Max(DefaultBounds.BoxExtent.X, DefaultBounds.BoxExtent.Y);
	const int TriesNumber = 2 * PI * ToSpawnRadius / BoundsRadius;
	TArray<float> AnglesToTry;
	const auto Angle = FMath::FRandRange(0.f, 360.f);
	for (float increment = 0.f; increment < 360.f; increment += 360.f / TriesNumber)
	{
		AnglesToTry.Add(Angle + increment);
	}

	const auto pPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!pPlayer)
		return nullptr;

	AActor* Actor = nullptr;
	for (const auto& AngleToTry : AnglesToTry)
	{
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
		Actor = SpawnActor<AActor>(Class, SpawnPosition, {}, SpawnParameters, true);
		if (Actor)
		{
			UpdateActiveZone();
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
