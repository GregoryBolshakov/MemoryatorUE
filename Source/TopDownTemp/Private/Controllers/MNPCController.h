#pragma once

#include "AIController.h"
#include "Characters/MCharacterSpecificTypes.h"
#include "MNPCController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRetreat);

class UMStateModelComponent;
class UAIPerceptionComponent;

UENUM(BlueprintType)
enum class EMobBehaviors : uint8
{
	Idle = 0,
	Walk,
	Chase,
	Fight,
	Follow,
	Talk,
	Guard,
	Retreat,
	Hide,
	Communicate,
};

//~=============================================================================
/**
 * Base class of an AI controller of an NPC
 */
UCLASS()
class TOPDOWNTEMP_API AMNPCController : public AAIController
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void StopOccupying();

	UPROPERTY(BlueprintCallable)
	FOnRetreat OnRetreatDelegate;

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

	UFUNCTION(BlueprintCallable)
	void Disembark();

	/** Triggers by rotating, i.e. DeltaRotation is non zero */
	void OnTurnAround() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bEmbarked = false;

private:
	virtual void Tick(float DeltaSeconds) override;

	FRotator LastRotation;

	// Every time rotation changes, calculate the delta from previous frame
	FRotator RotationDelta;
};