#pragma once

#include "AIController.h"
#include "MInterfaceMobController.h"
#include "MVillagerMobController.generated.h"

class AMCharacter;

//~=============================================================================
/**
 * Manages an NPC's behavior in the game.
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMVillagerMobController : public AAIController, public IMInterfaceMobController
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;

private:

	virtual void DoIdleBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void DoWalkBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void DoRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetIdleBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetWalkBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void OnBehaviorChanged(AMCharacter& MyCharacter) override;

	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UPROPERTY()
	AActor* HomeBuilding;
};