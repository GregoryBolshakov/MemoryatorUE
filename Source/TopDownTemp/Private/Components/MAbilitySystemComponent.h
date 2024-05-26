#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "MAbilitySystemComponent.generated.h"

UCLASS()
class UMAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	bool bCharacterAbilitiesGiven = false;
};
