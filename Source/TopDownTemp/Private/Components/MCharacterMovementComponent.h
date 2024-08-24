#pragma once
#include "GameFramework/CharacterMovementComponent.h"
#include "MCharacterMovementComponent.generated.h"

UCLASS()
class UMCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Maximum height character can step up when moving slowly */
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0", ForceUnits="cm"))
	float MaxStepHeightSlowMovement = 20.f;

	/** Maximum height character can step up when sprinting */
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0", ForceUnits="cm"))
	float MaxStepHeightSprint = 50.f;
};
