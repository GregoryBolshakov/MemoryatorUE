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

static TAutoConsoleVariable<FString> CVarToSpawnNightmareLarge(
		TEXT("r.NightmareLarge"),
		FString("NightmareLarge"),
		TEXT("Defines the TSubclassOf of the mob with given name"),
		ECVF_ReadOnly
	);

static TAutoConsoleVariable<FString> CVarToSpawnVillager(
		TEXT("r.Villager"),
		FString("Villager"),
		TEXT("Defines the TSubclassOf of the mob with given name"),
		ECVF_ReadOnly
	);

UMConsoleCommandsWorld::UMConsoleCommandsWorld()
{
}

void UMConsoleCommandsWorld::SpawnMob(const FString& MobClassString, int Quantity)
{
	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				if (const auto Class = pWorldGenerator->GetClassToSpawn(FName(MobClassString)))
				{
					const auto pPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
					if (!pPlayer)
						return;

					FActorSpawnParameters EmptySpawnParameters;
					pWorldGenerator->SpawnActorInRadius<AActor>(Class, pPlayer->GetActorLocation(), FRotator::ZeroRotator, EmptySpawnParameters, 150.f, 150.f);
				}
			}
		}
	}
}

