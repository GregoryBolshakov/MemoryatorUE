#pragma once
#include "MGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Managers/MBlockGenerator.h"
#include "Managers/MCommunicationManager.h"
#include "Managers/MDropManager.h"
#include "Managers/MExperienceManager.h"
#include "Managers/MMetadataManager.h"
#include "Managers/MReputationManager.h"
#include "Managers/MWorldGenerator.h"
#include "Managers/RoadManager/MRoadManager.h"
#include "Managers/SaveManager/MSaveManager.h"
#include "UObject/ConstructorHelpers.h"

AMGameMode* AMGameMode::GetAMGameMode(const UObject* Caller)
{
	if (const auto World = Caller->GetWorld())
	{
		return Cast<AMGameMode>(UGameplayStatics::GetGameMode(World));
	}
	return nullptr;
}

AMWorldGenerator* AMGameMode::GetWorldGenerator(const UObject* Caller)
{
	if (const auto GameMode = GetAMGameMode(Caller))
	{
		return GameMode->GetWorldGenerator();
	}
	return nullptr;
}

UMMetadataManager* AMGameMode::GetMetadataManager(const UObject* Caller)
{
	if (const auto GameMode = GetAMGameMode(Caller))
	{
		return GameMode->GetMetadataManager();
	}
	return nullptr;
}

UMDropManager* AMGameMode::GetDropManager(const UObject* Caller)
{
	if (const auto GameMode = GetAMGameMode(Caller))
	{
		return GameMode->GetDropManager();
	}
	return nullptr;
}

UMReputationManager* AMGameMode::GetReputationManager(const UObject* Caller)
{
	if (const auto GameMode = GetAMGameMode(Caller))
	{
		return GameMode->GetReputationManager();
	}
	return nullptr;
}

UMExperienceManager* AMGameMode::GetExperienceManager(const UObject* Caller)
{
	if (const auto GameMode = GetAMGameMode(Caller))
	{
		return GameMode->GetExperienceManager();
	}
	return nullptr;
}

UMSaveManager* AMGameMode::GetSaveManager(const UObject* Caller)
{
	if (const auto GameMode = GetAMGameMode(Caller))
	{
		return GameMode->GetSaveManager();
	}
	return nullptr;
}

AMCommunicationManager* AMGameMode::GetCommunicationManager(const UObject* Caller)
{
	if (const auto World = Caller->GetWorld())
	{
		if (const auto GameMode = Cast<AMGameMode>(UGameplayStatics::GetGameMode(World)))
		{
			return GameMode->GetCommunicationManager();
		}
	}
	return nullptr;
}

UMRoadManager* AMGameMode::GetRoadManager(const UObject* Caller)
{
	if (const auto World = Caller->GetWorld())
	{
		if (const auto GameMode = Cast<AMGameMode>(UGameplayStatics::GetGameMode(World)))
		{
			return GameMode->GetRoadManager();
		}
	}
	return nullptr;
}
