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
					const auto ToSpawnRadius = 150.f; // TODO: make editable or configurable

					const int TriesNumber = 10; // TODO: make editable or configurable
					TArray<float> AnglesToTry;
					const auto Angle = FMath::FRandRange(0.f, 360.f);
					for (float increment = 0.f; increment < 360.f; increment += 360.f / TriesNumber)
					{
						AnglesToTry.Add(Angle + increment);
					}

					for (int i = 0; i < Quantity; ++i)
					{
						const AActor* Actor = nullptr;
						for (const auto& AngleToTry : AnglesToTry)
						{
							const auto ToSpawnHeight = 150.f; // TODO: make editable or configurable

							const FVector SpawnPositionOffset (
									ToSpawnRadius * FMath::Cos(FMath::DegreesToRadians(AngleToTry)),
									ToSpawnRadius * FMath::Sin(FMath::DegreesToRadians(AngleToTry)),
									ToSpawnHeight
								);

							if (const auto pPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
							{
								const FVector SpawnPosition = pPlayer->GetTransform().GetLocation() + SpawnPositionOffset;
								FActorSpawnParameters EmptySpawnParameters{};
								Actor = pWorldGenerator->SpawnActor<AActor>(Class.Get(), SpawnPosition, {}, EmptySpawnParameters);
								if (Actor)
								{
									pWorldGenerator->UpdateActiveZone();
									break;
								}
							}
						}
						// If check is failed, consider incrementing ToSpawnRadius
						check(Actor != nullptr);
					}
				}
			}
		}
	}
}

