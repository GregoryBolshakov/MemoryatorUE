// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MCharacter.generated.h"

class UMAttackPuddleComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReverseMovementStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReverseMovementStopped);

UCLASS(Blueprintable)
class AMCharacter : public ACharacter
{
	GENERATED_UCLASS_BODY()

public:

	bool GetIsFighting() const { return IsFighting; }

	float GetSightRange() const { return SightRange; }

	float GetFightRange() const { return FightRange; }

	float GetForgetEnemyRange() const { return ForgetEnemyRange; }

	float GetRetreatRange() const { return RetreatRange; }

	float GetWalkSpeed() const { return WalkSpeed; }

	float GetRunSpeed() const { return RunSpeed; }

	bool GetCanRetreat() const { return bCanRetreat; }

	FVector GetForcedGazeVector() const { return ForcedGazeVector; }

	class UMIsActiveCheckerComponent* GetIsActiveCheckerComponent() const { return IsActiveCheckerComponent; }

	class UMInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	FVector GetLastNonZeroVelocity() const { return LastNonZeroVelocity; }

	bool GetIsDashing() const { return IsDashing; }

	void SetIsMoving(bool bIsMoving) { IsMoving = bIsMoving; UpdateAnimation(); }

	UFUNCTION(BlueprintCallable)
	void SetIsFighting(bool bIsFighting) { IsFighting = bIsFighting; UpdateAnimation(); }

	void SetIsDashing(bool bIsDashing) { IsDashing = bIsDashing; }

	void SetForcedGazeVector(FVector Vector) { ForcedGazeVector = Vector; }

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void UpdateAnimation();

	virtual void PostInitializeComponents() override;

protected:

	void UpdateLastNonZeroDirection();

	virtual void BeginPlay() override;

	/** Representation (collection of sprites) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = MCharacterComponents, meta = (AllowPrivateAccess = "true"))
	class UM2DRepresentationComponent* RepresentationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = MCharacterComponents, meta = (AllowPrivateAccess = "true"))
	UMInventoryComponent* InventoryComponent;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMIsActiveCheckerComponent* IsActiveCheckerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UMAttackPuddleComponent* AttackPuddleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector LastNonZeroVelocity = FVector(1.f, 0.f, 0.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector ForcedGazeVector;

	UPROPERTY(BlueprintAssignable, Category = "MRotatableFlipbookComponent", meta=(AllowPrivateAccess=true))
	FOnReverseMovementStarted OnReverseMovementStartedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "MRotatableFlipbookComponent", meta=(AllowPrivateAccess=true))
	FOnReverseMovementStopped OnReverseMovementStoppedDelegate;

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
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsDashing;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float Health;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float SightRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float FightRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float ForgetEnemyRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float Strength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float RetreatRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float WalkSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float RunSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	bool bCanRetreat;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float MeleeSpread;
	//TODO: Create variables for original values e.g. MaxHealth, DefaultSightRange, etc.
};

