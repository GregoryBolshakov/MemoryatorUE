#include "MReputationManager.h"

FReputation UMReputationManager::GetReputation(EFaction IN_Faction)
{
	if (const auto Reputation = ReputationMap.Find(IN_Faction))
	{
		return *Reputation;
	}
	return {0, 0};
}

FString UMReputationManager::FactionToString(const EFaction Faction)
{
	const UEnum* EnumPtr = FindObject<UEnum>(GetOuter(), TEXT("EFaction"), true);
	if (!EnumPtr) return "";

	return EnumPtr->GetNameStringByValue(static_cast<int64>(Faction));
}

TArray<EFaction> UMReputationManager::GetAllFactions()
{
	TArray<EFaction> Result;
	for (EFaction Faction : TEnumRange<EFaction>())
	{
		Result.Push(Faction);
	}
	return Result;
}
