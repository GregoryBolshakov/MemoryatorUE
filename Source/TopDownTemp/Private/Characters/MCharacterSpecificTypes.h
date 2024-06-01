#pragma once

#include "CoreMinimal.h"
#include "MCharacterSpecificTypes.generated.h"

UENUM()
enum class ERelationType
{
	Neutral = 0,
	Enemy = 1,
	Friend = 2
};