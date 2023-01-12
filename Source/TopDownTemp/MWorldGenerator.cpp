// Fill out your copyright notice in the Description page of Project Settings.

#include "MWorldGenerator.h"

#include "Math/UnrealMathUtility.h"
#include "MGroundBlock.h"
#include "MActor.h" 
#include "MIsActiveCheckerComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

AMWorldGenerator::AMWorldGenerator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
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
	auto* pWorld = GetWorld();
	
	for (int x = -WorldSize.X / 2; x < WorldSize.X / 2; x += 400)
	{
		for (int y = -WorldSize.Y / 2; y < WorldSize.Y / 2; y += 400)
		{
			FVector Location(x, y, 0);
			FRotator Rotation;
			FActorSpawnParameters SpawnParameters;
			//SpawnParameters.Name = FName(GetStringByClass<AMGroundBlock>() + "_" + FString::FromInt(x) + "_" + FString::FromInt(y));
			SpawnParameters.Name = FName(FString::FromInt(x) + FString::FromInt(y));

			auto* GroundBlock = pWorld->SpawnActor<AMGroundBlock>(ToSpawnGroundBlock, Location, Rotation, SpawnParameters);

			int TreeSpawnRate = FMath::RandRange(1, 5);
			//if (TreeSpawnRate == 1)
			{
				const auto TreeDefault = GetDefault<AMActor>(ToSpawnTree);
				FVector TreeLocation(x, y, 0);
				auto* Tree = pWorld->SpawnActor<AMActor>(ToSpawnTree, TreeLocation, Rotation);
			}
		}
	}
}

void AMWorldGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//TODO: Put the PlayerActiveZone handling to a separate function.
	const auto PlayerLocation = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetTransform().GetLocation();
	//TODO: Think how to fit the Active zone to the screen size
	PlayerActiveZone->SetWorldLocation(PlayerLocation);

	UClass* ActorClassFilter = AMActor::StaticClass();

	// Perform the overlap query
	TArray<AActor*> OverlappingActors;
	UKismetSystemLibrary::BoxOverlapActors(this, PlayerActiveZone->GetComponentLocation(), PlayerActiveZone->GetScaledBoxExtent(), { ObjectTypeQuery1 }, ActorClassFilter, {}, OverlappingActors);

	// Enable all the objects within PlayerActiveZone. ActiveActors is considered as from the previous check.
	for (const auto& Actor : OverlappingActors)
	{
		if (const auto IsActiveCheckerComponent = Actor->FindComponentByClass<UMIsActiveCheckerComponent>())
		{
			ActiveActors.Remove(Actor->GetName());
			IsActiveCheckerComponent->Enable();
		}
	}

	// All the rest of objects that were in PlayerActiveZone in the previous check but no longer there.
	for (auto& [Name, Actor] : ActiveActors)
	{
		const auto IsActiveCheckerComponent = Actor->FindComponentByClass<UMIsActiveCheckerComponent>();
		if (IsActiveCheckerComponent && IsActiveCheckerComponent->GetIsActive())
		{
			IsActiveCheckerComponent->Disable();
		}
	}

	// All and only OverlappingActors forms the new ActiveActors collection.
	ActiveActors.Empty();
	for (const auto& Actor : OverlappingActors)
	{
		ActiveActors.Add(Actor->GetName(), Actor);
	}
}
