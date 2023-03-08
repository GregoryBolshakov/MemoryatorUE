#pragma once
#include "MInterfaceMobController.generated.h"

class UWorld;
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

// Class needed to support InterfaceCast<class IInterfaceDriver>(Object)
UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint = true))
class UMInterfaceMobController : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

//~=============================================================================
/**
 * Interface for an AI controller of a mob
 */
class TOPDOWNTEMP_API IMInterfaceMobController
{
	GENERATED_IINTERFACE_BODY()

public:

	UFUNCTION(BlueprintCallable)
	virtual void OnFightEnd() {}

	UFUNCTION(BlueprintCallable)
	virtual void OnHit() {}

protected:

	virtual void DoIdleBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void DoWalkBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void DoChaseBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void DoFightBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void DoRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void SetIdleBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void SetWalkBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void SetChaseBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void SetFightBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void OnBehaviorChanged(AMCharacter& MyCharacter) {}

	EMobBehaviors CurrentBehavior = EMobBehaviors::Idle;
};