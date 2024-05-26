#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "MPlayerState.generated.h"

class UAbilitySystemComponent;

// TODO: Right now is not used. Should move AbilitySystemComponent here. Should also use it (player state) instead of MStateModelComponent.
UCLASS()
class AMPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMPlayerState();

	// Implement IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	FGameplayTag DeadTag;
	FGameplayTag KnockedDownTag;

	UPROPERTY()
	class UMAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY()
	class UMAttributeSetBase* AttributeSetBase;
};
