#include "MExperienceManager.h"
#include "StationaryActors/MPickableActor.h"

void UMExperienceManager::Initialize()
{
	// TODO: Make grpc calls to get the data for LevelsCost and ExperiencePerPickUp
}

float UMExperienceManager::GetExperiencePercent()
{
	if (LevelsCost.Num() < Level)
	{
		check(false);
		return 0.f;
	}

	return static_cast<float>(CurrentExperience) / LevelsCost[Level] * 100.f;
}

void UMExperienceManager::AddExperience(int Addition)
{
	const auto FinalAddition = Addition + Addition * ExperienceBoost;
	CurrentExperience += FinalAddition;

	while(CurrentExperience >= LevelsCost[Level])
	{
		CurrentExperience -= LevelsCost[Level];
		++Level;
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
