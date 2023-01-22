// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MCharacter.h"
#include "MMemoryator.generated.h"

UCLASS(Blueprintable)
class AMMemoryator : public AMCharacter
{
	GENERATED_UCLASS_BODY()

public:

	bool GetIsMoving() const { return IsMoving; }

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void UpdateAnimation();

	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.0f, bool bForce = false) override;

	virtual void PostInitializeComponents() override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns CursorToWorld subobject **/
	FORCEINLINE class UDecalComponent* GetCursorToWorld() { return CursorToWorld; }

private:

	void HandleCursor() const;

	void HandleAnimationStates();

	/** Representation (collection of sprites) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Representation, meta = (AllowPrivateAccess = "true"))
	class UM2DRepresentationComponent* RepresentationComponent;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMIsActiveCheckerComponent* IsActiveCheckerComponent;

	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** A decal that projects to the cursor location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Markers, meta = (AllowPrivateAccess = "true"))
	class UPaperSpriteComponent* DirectionMarkerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsDying;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsTakingDamage;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsFighting;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsMoving;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsPicking;
};

