#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TopDownTemp.h"
#include "MGameplayAbility.generated.h"

UCLASS()
class UMGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UMGameplayAbility();

	// Abilities with this set will automatically activate when the input is pressed
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Ability")
	EMAbilityInputID AbilityInputID = EMAbilityInputID::None;

	// Value to associate an ability with a slot without tying it to an automatically activated input.
	// Passive abilities won't be tied to an input, so we need a way to generically associate abilities with slots.
	// TODO: Figure out what that means
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Ability")
	EMAbilityInputID AbilityID = EMAbilityInputID::None;

	// TODO: Add the rest of the code
};
