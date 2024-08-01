#pragma once

#include "Animation/AnimInstance.h"
#include "MCharacterAnimInstance.generated.h"

UCLASS(Blueprintable)
class UMCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SpeedXY;

	/** We use discrete blend space levels to keep using BS, but never do actual blending.
	 * BS is much simpler to work with than separate locomotion states. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int MovementGridLevel = 0;

	UPROPERTY()
	float SprintSpeedRequirement = 750.f;

private:
	virtual void NativeBeginPlay() override;

	virtual void PreUpdateAnimation(float DeltaSeconds) override;
};
