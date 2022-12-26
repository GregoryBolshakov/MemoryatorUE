// Fill out your copyright notice in the Description page of Project Settings.

#include "MWorldGenerator.h"

#include "Math/UnrealMathUtility.h"
#include "MGroundBlock.h"
#include "MActor.h" 
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"

void UMWorldGenerator::GenerateWorld()
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

			Objects.Add(SpawnParameters.Name, GroundBlock);

			int TreeSpawnRate = FMath::RandRange(1, 5);
			//if (TreeSpawnRate == 1)
			{
				const auto TreeDefault = GetDefault<AMActor>(ToSpawnTree);
				FVector TreeLocation(x, y, 0);
				auto* Tree = pWorld->SpawnActor<AMActor>(ToSpawnTree, TreeLocation, Rotation);
				//Objects.Add(, Tree);
			}
		}
	}
}
