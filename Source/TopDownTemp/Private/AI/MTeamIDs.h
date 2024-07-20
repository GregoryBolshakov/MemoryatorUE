#pragma once
#include "GenericTeamAgentInterface.h"

UENUM(BlueprintType)
enum class EMTeamID : uint8
{
	None UMETA(DisplayName = "None"),
	Humans UMETA(DisplayName = "Humans"),
	Nightmares UMETA(DisplayName = "Nightmares"),
	Witches UMETA(DisplayName = "Witches"),
	// Add more teams as needed
};

static FGenericTeamId GetTeamIdByEnum(EMTeamID TeamID)
{
	return FGenericTeamId(static_cast<int>(TeamID));
}
