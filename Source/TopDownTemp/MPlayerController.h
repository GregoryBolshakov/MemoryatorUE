// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MPlayerController.generated.h"

UCLASS()
class AMPlayerController : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:

	bool IsMovingByAI() const;

	void StopAIMovement();

private:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;
	bool bIsTurningAround = false;

	UPROPERTY()
	class UPathFollowingComponent* PathFollowingComponent;

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

	/** Input handlers for SetDestination action. */
	void OnSetDestinationPressed();
	void OnSetDestinationReleased();
	
	void OnToggleTurnAroundPressed();
	void OnToggleTurnAroundReleased();
};


