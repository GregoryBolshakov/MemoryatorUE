// Copyright Epic Games, Inc. All Rights Reserved.

#include "MConsoleCommandsWorld.h"
#include "MWorldManager.h"
#include "MWorldGenerator.h"
#include "Kismet/GameplayStatics.h"

static TAutoConsoleVariable<FString> CVarToSpawnNightmare(
		TEXT("r.Nightmare"), 
		FString("Nightmare"),
		TEXT("Defines the TSubclassOf of the mob with given name"),
		ECVF_ReadOnly
	);

static TAutoConsoleVariable<FString> CVarToSpawnNightmareMedium(
		TEXT("r.NightmareMedium"), 
		FString("NightmareMedium"),
		TEXT("Defines the TSubclassOf of the mob with given name"),
		ECVF_ReadOnly
	);

UMConsoleCommandsWorld::UMConsoleCommandsWorld()
{
}

void UMConsoleCommandsWorld::SpawnMob(const FString& MobClassString)
{
	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				if (const auto Class = pWorldGenerator->GetClassToSpawn(FName(MobClassString)))
				{
					const auto ToSpawnRadius = 150.f;
					const auto Angle = FMath::FRandRange(0.f, 360.f);;
					const auto ToSpawnHeight = 150.f;

					const FVector SpawnPositionOffset (
							ToSpawnRadius * FMath::Cos(FMath::DegreesToRadians(Angle)),
							ToSpawnRadius * FMath::Sin(FMath::DegreesToRadians(Angle)),
							ToSpawnHeight
						);

					if (const auto pPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
					{
						const FVector SpawnPosition = pPlayer->GetTransform().GetLocation() + SpawnPositionOffset;
						pWorldGenerator->SpawnActor<AActor>(Class.Get(), SpawnPosition, {}, "");
						pWorldGenerator->UpdateActiveZone();
					}
				}
			}
		}
	}
}

