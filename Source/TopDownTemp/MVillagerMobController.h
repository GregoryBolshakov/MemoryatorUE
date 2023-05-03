#pragma once

#include "MMobControllerBase.h"
#include "MVillagerMobController.generated.h"

class AMCharacter;

//~=============================================================================
/**
 * Manages an NPC's behavior in the game.
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMVillagerMobController : public AMMobControllerBase
{
	GENERATED_UCLASS_BODY()

public:
	virtual void PreTick(float DeltaSeconds, const UWorld& World, AMCharacter& MyCharacter) override;

	void Initialize(AActor& HomeBuilding, const FVector& VillageCenter, float VillageRadius);

private:

	virtual void DoIdleBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void DoWalkBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void DoRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void DoHideBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetIdleBehavior(const UWorld* World, AMCharacter* MyCharacter) override;

	virtual void SetWalkBehavior(const UWorld& World, AMCharacter& MyCharacter, const FVector& DestinationPoint) override;

	virtual void SetRetreatBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void SetHideBehavior(const UWorld& World, AMCharacter& MyCharacter) override;

	virtual void OnBehaviorChanged(AMCharacter& MyCharacter) override;

	void Embark(const AMCharacter& MyCharacter);

	void Disembark(const AMCharacter& MyCharacter);

	UPROPERTY()
	AActor* HomeBuilding;

	FVector VillageCenter;

	float VillageRadius;

	/** Min time of doing nothing after reaching destination point */
	UPROPERTY(EditAnywhere, Category = BehaviorParameters, meta = (AllowPrivateAccess = true))
	float MinRestDuration = 1.5f;

	/** Max time of doing nothing after reaching destination point */
	UPROPERTY(EditAnywhere, Category = BehaviorParameters, meta = (AllowPrivateAccess = true))
	float MaxRestDuration = 4.5f;

	FTimerHandle RestTimerHandle;

	UPROPERTY()
	TMap<FName, AActor*> EnemiesNearby;

	bool bEmbarked;
};