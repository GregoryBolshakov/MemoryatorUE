#pragma once

#include "AIController.h"
#include "MInterfaceMobController.h"
#include "MMobController.generated.h"

class AMCharacter;

//~=============================================================================
/**
 * Manages an NPC's behavior in the game. 
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMMobController : public AAIController, public IMInterfaceMobController
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;

	virtual void OnFightEnd() override;

	virtual void OnHit() override;

private:

	virtual void DoIdleBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void DoChaseBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void DoFightBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void DoRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetIdleBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetChaseBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetFightBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void OnBehaviorChanged(AMCharacter& MyCharacter) override;

	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UPROPERTY()
	APawn* Victim;
};