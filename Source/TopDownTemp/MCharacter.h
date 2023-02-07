// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MCharacter.generated.h"

UCLASS(Blueprintable)
class AMCharacter : public ACharacter
{
	GENERATED_UCLASS_BODY()

public:

	bool GetIsFighting() const { return IsFighting; }

	float GetSightRange() const { return SightRange; }

	float GetFightRange() const { return FightRange; }

	float GetRetreatRange() const { return RetreatRange; }
	
	bool GetCanRetreat() const { return bCanRetreat; }

	TSubclassOf<class AAIController> GetControllerClass() const { return ControllerClass; }

	void SetIsFighting(bool bIsFighting) { IsFighting = bIsFighting; }

	void SetForcedGazeVector(FVector Vector) { ForcedGazeVector = Vector; }

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void UpdateAnimation();

	virtual void PostInitializeComponents() override;

protected:
	virtual void HandleAnimationStates();

	void UpdateLastNonZeroDirection();

	/** Representation (collection of sprites) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Representation, meta = (AllowPrivateAccess = "true"))
	class UM2DRepresentationComponent* RepresentationComponent;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMIsActiveCheckerComponent* IsActiveCheckerComponent;

	FVector LastNonZeroVelocity = FVector(1.f, 0.f, 0.f);
	FVector ForcedGazeVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Controller, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AAIController> ControllerClass;

	//TODO: It's hard to say if these booleans should be here or in Controller
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float Health;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float SightRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float FightRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float Strength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float RetreatRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	bool bCanRetreat;
	//TODO: Create variables for original values e.g. MaxHealth, DefaultSightRange, etc.
};

