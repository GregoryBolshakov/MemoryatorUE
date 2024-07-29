#pragma once

#include "AIController.h"
#include "Characters/MCharacterSpecificTypes.h"
#include "MInterfaceMobController.h"
#include "MNPCController.generated.h"

class UMStateModelComponent;
class UAIPerceptionComponent;

//~=============================================================================
/**
 * Base class of an AI controller of an NPC
 */
UCLASS()
class TOPDOWNTEMP_API AMNPCController : public AAIController
{
	GENERATED_UCLASS_BODY()

public:

protected: // IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	void OnUnPossess() override;

	UFUNCTION()
	void CopyStateVariablesToBlackboard(const UMStateModelComponent* StateModel);

	UFUNCTION(BlueprintCallable)
	void Embark();

	/** Triggers by rotating, i.e. DeltaRotation is non zero */
	void OnTurnAround() const;

private:
	virtual void Tick(float DeltaSeconds) override;

	bool bEmbarked = false;

	FRotator LastRotation;

	// Every time rotation changes, calculate the delta from previous frame
	FRotator RotationDelta;
};