// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MCharacter.generated.h"

class UMBuffManagerComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReverseMovementStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReverseMovementStopped);

UCLASS(Blueprintable)
class AMCharacter : public ACharacter
{
	GENERATED_UCLASS_BODY()

public:

	float GetRadius() const;

	bool GetIsFighting() const { return IsFighting; }

	float GetSightRange() const { return SightRange; }

	float GetFightRangePlusMyRadius() const { return FightRange + GetRadius(); }

	float GetForgetEnemyRange() const { return ForgetEnemyRange; }

	float GetRetreatRange() const { return RetreatRange; }

	float GetWalkSpeed() const { return WalkSpeed; }

	float GetSprintSpeed() const { return SprintSpeed; }

	float GetStrength() const { return Strength; }

	bool GetIsSprinting() const { return IsSprinting; }

	bool GetCanRetreat() const { return bCanRetreat; }

	float GetTimeBeforeSprint() const { return TimeBeforeSprint; }

	FVector GetForcedGazeVector() const { return ForcedGazeVector; }

	class UMIsActiveCheckerComponent* GetIsActiveCheckerComponent() const { return IsActiveCheckerComponent; }

	class UMInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	class UMAttackPuddleComponent* GetAttackPuddleComponent() const { return AttackPuddleComponent; }

	class UMCommunicationComponent* GetCommunicationComponent() const { return CommunicationComponent; }

	FVector GetLastNonZeroVelocity() const { return LastNonZeroVelocity; }

	bool GetIsDashing() const { return IsDashing; }

	void SetIsMoving(bool bIsMoving) { IsMoving = bIsMoving; UpdateAnimation(); }

	UFUNCTION(BlueprintCallable)
	void SetIsFighting(bool bIsFighting) { IsFighting = bIsFighting; UpdateAnimation(); }

	void SetIsDashing(bool bIsDashing) { IsDashing = bIsDashing; }

	void SetIsSprinting(bool IN_bIsSprinting) { IsSprinting = IN_bIsSprinting; }

	void SetForcedGazeVector(FVector Vector) { ForcedGazeVector = Vector; }

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void UpdateAnimation();

	virtual void PostInitializeComponents() override;

protected:

	void UpdateLastNonZeroDirection();

	virtual void BeginPlay() override;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Representation (collection of sprites) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = MCharacterComponents, meta = (AllowPrivateAccess = "true"))
	class UM2DRepresentationComponent* RepresentationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UMInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UMCommunicationComponent* CommunicationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMIsActiveCheckerComponent* IsActiveCheckerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UMBuffBarComponent* BuffBarComponent;

	//TODO: Consider creating separate manager for all underfoot UI
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UMAttackPuddleComponent* AttackPuddleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UPaperSpriteComponent* PerimeterOutlineComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector LastNonZeroVelocity = FVector(1.f, 0.f, 0.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector ForcedGazeVector;

	UPROPERTY(BlueprintAssignable, Category = "MRotatableFlipbookComponent", meta=(AllowPrivateAccess=true))
	FOnReverseMovementStarted OnReverseMovementStartedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "MRotatableFlipbookComponent", meta=(AllowPrivateAccess=true))
	FOnReverseMovementStopped OnReverseMovementStoppedDelegate;

	//TODO: create a separate entity to store this. Now it needs to be set for each ancestor (it's bad).
	UPROPERTY(EditDefaultsOnly, Category=MBuffManagerComponent)
	TSubclassOf<UUserWidget> BuffBarWidgetBPClass;

	//TODO: It's hard to say if these booleans should be here or in Controller
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsDying = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsTakingDamage = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsFighting = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsMoving = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsPicking = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsDashing = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AnimationState, meta = (AllowPrivateAccess = "true"))
	bool IsSprinting = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float Health;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float SightRange = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float FightRange = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float ForgetEnemyRange = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float Strength = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float RetreatRange = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	bool bCanRetreat = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float MeleeSpread = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float WalkSpeed = 185.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float SprintSpeed = 260.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float TimeBeforeSprint = 1.f;

	//TODO: Create variables for original values e.g. MaxHealth, DefaultSightRange, etc.
};

