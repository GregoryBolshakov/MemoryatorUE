#pragma once
#include "MInterfaceMobController.generated.h"

class UWorld;
class AMCharacter;

UENUM(BlueprintType)
enum class EMobBehaviors : uint8
{
	Idle = 0,
	Walk,
	Chase,
	Fight,
	Follow,
	Speak,
	Guard,
	Retreat,
	Hide,
	Communicate,
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
	virtual void OnFightAnimationEnd() {}

	/** The possessed mob hit somebody */
	UFUNCTION(BlueprintCallable)
	virtual void OnHit() {}

protected:

	virtual void DoIdleBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void DoWalkBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void DoChaseBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void DoFightBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void DoRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void DoHideBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void DoCommunicationBehavior(const UWorld& World, AMCharacter& MyCharacter) {} 

	UFUNCTION(BlueprintCallable)
	virtual void SetIdleBehavior(const UWorld* World, AMCharacter* MyCharacter) {}

	virtual void SetWalkBehavior(const UWorld& World, AMCharacter& MyCharacter, const FVector& DestinationPoint) {}

	virtual void SetChaseBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void SetFightBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void SetHideBehavior(const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void SetCommunicationBehavior(const UWorld& World, AMCharacter& MyCharacter) {} 

	/** Logic to do in transition between behaviors */
	virtual void OnBehaviorChanged(AMCharacter& MyCharacter) {}
};