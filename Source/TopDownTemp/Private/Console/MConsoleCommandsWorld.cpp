#include "MConsoleCommandsWorld.h"

#include "Framework/MGameMode.h"
#include "Managers/MWorldGenerator.h"
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

static TAutoConsoleVariable<FString> CVarToSpawnPeasant(
		TEXT("r.Peasant"),
		FString("Peasant"),
		TEXT("Defines the TSubclassOf of the mob with given name"),
		ECVF_ReadOnly
	);

UMConsoleCommandsWorld::UMConsoleCommandsWorld()
{
}

void UMConsoleCommandsWorld::SpawnMob(const FString& MobClassString, int Quantity)
{
	if (const auto WorldGenerator = AMGameMode::GetWorldGenerator(this))
	{
		if (const auto Class = WorldGenerator->GetClassToSpawn(FName(MobClassString)))
		{
			const auto pPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
			if (!pPlayer)
				return;

			WorldGenerator->SpawnActorInRadius<AActor>(Class, pPlayer->GetActorLocation(), FRotator::ZeroRotator, {}, 150.f, 350.f);
		}
	}
}

