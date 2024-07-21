#pragma once

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "MStateModelComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStateDirty);

/** Component that stores and replicates all boolean states e.g. IsDashing, IsTakingDamage, IsDying, etc.\n
 * Supposed to be attached to AMCharacter or AMActor */
// TODO: Supposedly will be using GAS attributes instead
UCLASS(BlueprintType)
class UMStateModelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	bool GetIsDirty() const { return IsDirty; }
	void CleanDirty() { IsDirty = false; }

	UFUNCTION(BlueprintCallable)
	bool GetIsCommunicating() const { return IsCommunicating; }
	
	UFUNCTION(BlueprintCallable)
	bool GetIsDashing() const { return IsDashing; }

	UFUNCTION(BlueprintCallable)
	bool GetIsFighting() const { return IsFighting; }

	UFUNCTION(BlueprintCallable)
	bool GetIsMoving() const { return IsMoving; }

	UFUNCTION(BlueprintCallable)
	bool GetIsReversing() const { return IsReversing; }

	UFUNCTION(BlueprintCallable)
	bool GetIsSprinting() const { return IsSprinting; }

	UFUNCTION(BlueprintCallable)
	bool GetIsTakingDamage() const { return IsTakingDamage; }

	UFUNCTION(BlueprintCallable)
	bool GetIsTurningLeft() const { return IsTurningLeft; }

	UFUNCTION(BlueprintCallable)
	bool GetIsTurningRight() const { return IsTurningRight; }

	UFUNCTION(BlueprintCallable)
	inline void SetIsCommunicating(bool IN_IsCommunicating);

	UFUNCTION(BlueprintCallable)
	inline void SetIsDashing(bool IN_IsDashing);

	UFUNCTION(BlueprintCallable)
	inline void SetIsFighting(bool IN_IsFighting);

	UFUNCTION(BlueprintCallable)
	inline void SetIsMoving(bool IN_IsMoving);

	UFUNCTION(BlueprintCallable)
	inline void SetIsReversing(bool IN_IsReversing);

	UFUNCTION(BlueprintCallable)
	inline void SetIsSprinting(bool IN_IsSprinting);

	UFUNCTION(BlueprintCallable)
	inline void SetIsTakingDamage(bool IN_IsTakingDamage);

	UFUNCTION(BlueprintCallable)
	inline void SetIsTurningLeft(bool IN_IsTurningLeft);

	UFUNCTION(BlueprintCallable)
	inline void SetIsTurningRight(bool IN_IsTurningRight);

	UPROPERTY()
	FOnStateDirty OnDirtyDelegate;

protected:
	// TODO: Figure out why making all properties using OnRep fixes the bug with inconsistent updates for client
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty)
	bool IsDirty = true;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere)
	bool IsCommunicating = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere)
	bool IsDashing = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere)
	bool IsDying = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere)
	bool IsFighting = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere)
	bool IsMoving = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere)
	bool IsPicking = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere)
	bool IsReversing = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere)
	bool IsSprinting = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere)
	bool IsTakingDamage = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere)
	bool IsTurningLeft = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere)
	bool IsTurningRight = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);

		DOREPLIFETIME(UMStateModelComponent, IsDirty);
		DOREPLIFETIME(UMStateModelComponent, IsCommunicating);
		DOREPLIFETIME(UMStateModelComponent, IsDashing);
		DOREPLIFETIME(UMStateModelComponent, IsDying);
		DOREPLIFETIME(UMStateModelComponent, IsFighting);
		DOREPLIFETIME(UMStateModelComponent, IsMoving);
		DOREPLIFETIME(UMStateModelComponent, IsPicking);
		DOREPLIFETIME(UMStateModelComponent, IsReversing);
		DOREPLIFETIME(UMStateModelComponent, IsSprinting);
		DOREPLIFETIME(UMStateModelComponent, IsTakingDamage);
		DOREPLIFETIME(UMStateModelComponent, IsTurningLeft);
		DOREPLIFETIME(UMStateModelComponent, IsTurningRight);
	}

	UFUNCTION()
	void OnRep_IsDirty()
	{
		OnDirtyDelegate.Broadcast();
	}
};

#if CPP
#include "MStateModelComponent.inl"
#endif
