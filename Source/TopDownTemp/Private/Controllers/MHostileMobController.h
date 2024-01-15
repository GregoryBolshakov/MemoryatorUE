#pragma once

#include "MMobControllerBase.h"
#include "MHostileMobController.generated.h"

class AMOutpostHouse;
class AMCharacter;

//~=============================================================================
/**
 * Manages a hostile NPC's behavior in the game. Should be refactored in the future, currently needed for prototyping
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMHostileMobController : public AMMobControllerBase
{
	GENERATED_UCLASS_BODY()

public:
	virtual void OnFightAnimationEnd() override;

	virtual void OnHit() override;

	AMOutpostHouse* GetHouse() const { return House; }

private:
	virtual void DoIdleBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void DoChaseBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void DoFightBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void DoRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetIdleBehavior(const UWorld* World, AMCharacter* MyCharacter) override;

	virtual void SetChaseBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetFightBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void OnBehaviorChanged(AMCharacter& MyCharacter) override;

	UPROPERTY()
	APawn* Victim;
};