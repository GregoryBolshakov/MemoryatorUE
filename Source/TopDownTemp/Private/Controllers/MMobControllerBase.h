#pragma once

#include "AIController.h"
#include "MInterfaceMobController.h"

#include "MMobControllerBase.generated.h"

class UWorld;
class AMCharacter;

//TODO: Consider adding FAIRequestID RequestID, const FPathFollowingResult& Result parameters
DECLARE_DELEGATE(FOnMoveCompleted);

//~=============================================================================
/**
 * Base class of an AI controller of a mob
 */
UCLASS()
class TOPDOWNTEMP_API AMMobControllerBase : public AAIController, public IMInterfaceMobController
{
	GENERATED_UCLASS_BODY()

protected:

	/** High priority logic to be performed before behavior processing. I.e. check for enemies nearby */
	virtual void PreTick(float DeltaSeconds, const UWorld& World, AMCharacter& MyCharacter) {}

	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	bool IsPlayerSpeakingToMe(); //TODO: Move to the AMCommunicationManager

	ERelationType GetRelationshipWithPlayer();

	/** Represents relationship with other pawns. Neutral if not listed */
	UPROPERTY(EditAnywhere, Category = BehaviorParameters, meta=(AllowPrivateAccess = true))
	TMap<TSubclassOf<APawn>, ERelationType> RelationshipMap;

	UPROPERTY(VisibleAnywhere, Category = BehaviorParameters, meta=(AllowPrivateAccess = true))
	EMobBehaviors CurrentBehavior = EMobBehaviors::Idle;

	FOnMoveCompleted OnMoveCompletedDelegate;

private:

	virtual void Tick(float DeltaSeconds) override;

};