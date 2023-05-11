// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Components/TimelineComponent.h"
#include "MPlayerController.generated.h"

class AMCharacter;
class UCurveFloat;
enum class ERelationType;

UCLASS()
class AMPlayerController : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:

	bool IsMovingByAI() const;

	void StopAIMovement();

	void StartSprintTimer();

	void TurnSprintOn();

	UFUNCTION(BlueprintCallable)
	void TurnSprintOff();

	const TMap<TSubclassOf<APawn>, ERelationType>& GetRelationshipMap() const { return RelationshipMap; }

//Dash
protected:

	UFUNCTION()
	void TimelineProgress(float Value);

	void OnDashPressed();

	UPROPERTY(EditDefaultsOnly, Category="Dash")
	float DashLength = 135.f;

	FTimeline DashVelocityTimeline;

	UPROPERTY(EditAnywhere, Category = "Dash")
	UCurveFloat* DashVelocityCurve;

	FDateTime TimeSinceLastDashUpdate;
	float LastDashProgressValue;

// Interaction with other mobs
protected:

	void SetDynamicActorsNearby(const UWorld& World, AMCharacter& MyCharacter);

	void UpdateClosestEnemy(AMCharacter& MyCharacter);

	UFUNCTION(BlueprintCallable)
	void OnHit();

	/** Represents relationship with other pawns. Neutral if not listed */
	UPROPERTY(EditAnywhere, Category = BehaviorParameters, meta=(AllowPrivateAccess = true))
	TMap<TSubclassOf<APawn>, ERelationType> RelationshipMap;

	UPROPERTY()
	TMap<FName, AActor*> EnemiesNearby;

	UPROPERTY()
	AActor* ClosestEnemy;

	FTimerHandle ActorsNearbyUpdateTimerHandle;

protected:

	virtual void BeginPlay() override;

private:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;
	bool bIsTurningAround = false;

	UPROPERTY()
	class UPathFollowingComponent* PathFollowingComponent;

	UPROPERTY(EditDefaultsOnly)
	class UMConsoleCommandsManager* ConsoleCommandsManager;

	// Begin PlayerController interface
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

	void MoveRight(float Value);
	void MoveForward(float Value);

	void TurnAround(float Value);

	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();

	/** Navigate player to the current touch location. */
	void MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location);

	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	//TODO: Figure out of we're gonna need this at all
	/** Input handlers for SetDestination action. */
	//void OnSetDestinationPressed();
	//void OnSetDestinationReleased();

	void OnToggleTurnAroundPressed();
	void OnToggleTurnAroundReleased();

	void OnToggleFightPressed();

	void OnLeftMouseClick();

	virtual bool ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor) override;
};


