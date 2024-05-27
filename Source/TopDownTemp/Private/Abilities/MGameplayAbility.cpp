#include "MGameplayAbility.h"
#include "AbilitySystemComponent.h"

UMGameplayAbility::UMGameplayAbility()
{
	// Default to Instance Per Actor
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// UGSAbilitySystemGlobals hasn't initialized tags yet to set ActivationBlockedTags
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Dead"));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.KnockedDown"));
}
