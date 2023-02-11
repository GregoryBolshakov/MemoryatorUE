// Fill out your copyright notice in the Description page of Project Settings.

#include "MWorldGenerator.h"

#include "Math/UnrealMathUtility.h"
#include "MGroundBlock.h"
#include "MActor.h" 
#include "MCharacter.h"
#include "MMemoryator.h"
#include "MAICrowdManager.h"
#include "MIsActiveCheckerComponent.h"
#include "MWorldManager.h"
#include "NavigationSystem.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavMesh/NavMeshBoundsVolume.h"

AMWorldGenerator::AMWorldGenerator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DynamicActorsCheckInterval(0.5f)
	, DynamicActorsCheckTimer(0.f)
	, UniqueNameSuffix(0)
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

	auto ToSpawnGroundBlock = ToSpawnActorMap.Find(FName("GroundBlock"));
	const auto GroundBlockSize = Cast<AMGroundBlock>(ToSpawnGroundBlock->GetDefaultObject())->GetSize();

	for (int x = -WorldSize.X / 2; x < WorldSize.X / 2; x += GroundBlockSize.X)
	{
		for (int y = -WorldSize.Y / 2; y < WorldSize.Y / 2; y += GroundBlockSize.Y)
		{
			FVector Location(x, y, 0);

			auto* GroundBlock = SpawnActor<AMGroundBlock>(ToSpawnGroundBlock->Get(), Location, {}, FName(FString::FromInt(x) + FString::FromInt(y)));

			int TreeSpawnRate = FMath::RandRange(1, 5);
			//if (TreeSpawnRate == 1)
			{
				const auto TreeDefault = GetDefault<AMActor>(*ToSpawnActorMap.Find(FName("Tree")));
				FVector TreeLocation(x, y, 0);

				auto* Tree = SpawnActor<AMActor>(*ToSpawnActorMap.Find(FName("Tree")), TreeLocation, {}, FName("Tree_" + FString::FromInt(x) + FString::FromInt(y)));
			}
		}
	}
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
							IsActiveCheckerComponent->Enable();
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
							IsActiveCheckerComponent->Enable();
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
						IsActiveCheckerComponent->Disable();
					}
				}
			}
			for (const auto& [Index, Data] : Block->DynamicActors)
			{
				if (Data) // temporary
				{
					if (const auto IsActiveCheckerComponent = Data->FindComponentByClass<UMIsActiveCheckerComponent>())
					{
						IsActiveCheckerComponent->Disable();
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

FIntPoint AMWorldGenerator::GetGroundBlockIndex(FVector Position) const
{
	if (const auto ToSpawnGroundBlock = ToSpawnActorMap.Find(FName("GroundBlock")))
	{
		if (const auto GroundBlock = Cast<AMGroundBlock>(ToSpawnGroundBlock->GetDefaultObject()))
		{
			const auto GroundBlockSize = GroundBlock->GetSize();
			return FIntPoint(UE4::SSE::FloorToInt32(Position.X/GroundBlockSize.X), UE4::SSE::FloorToInt32(Position.Y/GroundBlockSize.Y));
		}
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

AActor* AMWorldGenerator::SpawnActor(UClass* Class, FVector const& Location, FRotator const& Rotation,
                                     const FActorSpawnParameters& SpawnParameters)
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

	const auto Actor = pWorld->SpawnActor(Class, &Location, &Rotation, SpawnParameters);

	if (bDynamic)
	{
		if (const auto Character = Cast<AMCharacter>(Actor))
		{
			// Spawn an AI controller for a spawned creature
			if (const auto pCrowdManager = pWorld->GetSubsystem<UMWorldManager>()->GetCrowdManager())
			{
				if (const auto Controller = pCrowdManager->SpawnAIController(SpawnParameters.Name, Character->GetControllerClass(), Location, Rotation)) //TODO: Add valid spawn parameters
				{
					Controller->Possess(Character);
				}
				else
				{
					check(false);
				}
			}
		}
	}

	// We also store the mapping between the Name and metadata (actor's GroundBlock index, etc.)
	const FActorWorldMetadata Metadata{ListToAdd.Add(SpawnParameters.Name, Actor), GroundBlockIndex};
	ActorsMetadata.Add(SpawnParameters.Name, Metadata);

	return Metadata.Actor;
}

TSubclassOf<AActor> AMWorldGenerator::GetClassToSpawn(FName Name)
{
	if (const auto Class = ToSpawnActorMap.Find(Name))
	{
		return *Class;
	}
	return nullptr;
}

TMap<FName, FActorWorldMetadata> AMWorldGenerator::GetActorsInRect(FVector UpperLeft, FVector BottomRight, bool bDynamic)
{
	const auto StartBlock = GetGroundBlockIndex(UpperLeft);
	const auto FinishBlock = GetGroundBlockIndex(BottomRight);

	TMap<FName, FActorWorldMetadata> Result;

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
						if (const auto Metadata = ActorsMetadata.Find(Name))
							Result.Add(Name, *Metadata);
					}
				}
			}
		}
	}

	return Result;
}
