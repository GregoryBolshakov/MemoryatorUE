#pragma once

#include "AIController.h"
#include "MMobController.generated.h"

class AMCharacter;

UENUM()
enum class EMobBehaviors
{
	Idle = 0,
	Chase,
	Fight,
	Follow,
	Speak,
	Guard,
	Retreat,
	Hide,
};

//~=============================================================================
/**
 * Manages an NPC's behavior in the game. 
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMMobController : public AAIController
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable)
	virtual void OnFightEnd();

	UFUNCTION(BlueprintCallable)
	virtual void OnHit();

private:

	void DoIdleBehavior(const UWorld& World, AMCharacter& MyCharacter);

	void DoChaseBehavior(const UWorld& World, AMCharacter& MyCharacter);

	void DoFightBehavior(const UWorld& World, AMCharacter& MyCharacter);

	void DoRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter);

	void SetIdleBehavior(const UWorld& World, AMCharacter& MyCharacter);

	void SetChaseBehavior(const UWorld& World, AMCharacter& MyCharacter);

	void SetFightBehavior(const UWorld& World, AMCharacter& MyCharacter);

	void SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter);

	void OnBehaviorChanged(AMCharacter& MyCharacter);

	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UPROPERTY()
	EMobBehaviors CurrentBehavior;

	/** Default time between behavior decision in seconds */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Behavior, meta = (AllowPrivateAccess = "true"))
	float DefaultTimeBetweenDecisions;

	/** Time left before a new behavioral decision is made */
	float CurrentTimeBetweenDecisions;

	UPROPERTY()
	APawn* Victim;
};