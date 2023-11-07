#include "MExperienceManager.h"

#include "Framework/MGameInstance.h"
#include "StationaryActors/MPickableActor.h"
#include "NakamaManager/Private/NakamaManager.h"
#include "NakamaManager/Private/UserManagerClient.h"

FHero EmptyHero;
FHero* GetHeroPtr(UWorld* World)
{
	if (IsValid(World))
	{
		if (const auto pGameInstance = World->GetGameInstance<UMGameInstance>())
		{
			if (const auto NakamaManager = pGameInstance->GetNakamaManager())
			{
				if (const auto UserManager = NakamaManager->UserManagerClient)
				{
					return &UserManager->Hero;
				}
			}
		}
	}
	check(false);
	return &EmptyHero;
}

int UMExperienceManager::GetLevel()
{
	return GetHeroPtr(GetWorld())->Level;
}

int UMExperienceManager::GetCurrentExperience()
{
	return GetHeroPtr(GetWorld())->Experience;
}

float UMExperienceManager::GetExperiencePercent()
{
	if (LevelsCost.Num() < GetLevel())
	{
		check(false);
		return 0.f;
	}

	return static_cast<float>(GetCurrentExperience()) / LevelsCost[GetLevel()] * 100.f;
}

void UMExperienceManager::AddExperience(int Addition)
{
	const auto FinalAddition = Addition + Addition * ExperienceBoost;
	auto HeroRef = GetHeroPtr(GetWorld());
	HeroRef->Experience += FinalAddition;

	while(GetCurrentExperience() >= LevelsCost[HeroRef->Level])
	{
		HeroRef->Experience -= LevelsCost[HeroRef->Level];
		HeroRef->Level++;
	}

	ExperienceAddedDelegate.Broadcast(FinalAddition);
}

void UMExperienceManager::OnActorPickedUp(TSubclassOf<AMPickableActor> IN_Class)
{
	if (const auto pExpReward = ExperiencePerPickUp.Find(IN_Class))
	{
		AddExperience(*pExpReward);
	}
}
