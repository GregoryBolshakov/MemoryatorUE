#pragma once

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "MStateModelComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStateDirty);

/** Component that stores and replicates all boolean states e.g. IsDashing, IsTakingDamage, IsDying, etc.\n
 * Supposed to be attached to AMCharacter or AMActor */
UCLASS(BlueprintType)
class UMStateModelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	bool GetIsDirty() const { return IsDirty; }
	void CleanDirty() { IsDirty = false; }

	UFUNCTION(BlueprintCallable)
	bool GetIsDashing() const { return IsDashing; }

	UFUNCTION(BlueprintCallable)
	bool GetIsFighting() const { return IsFighting; }

	UFUNCTION(BlueprintCallable)
	bool GetIsMoving() const { return IsMoving; }

	UFUNCTION(BlueprintCallable)
	bool GetIsSprinting() const { return IsSprinting; }

	UFUNCTION(BlueprintCallable)
	bool GetIsTakingDamage() const { return IsTakingDamage; }

	UFUNCTION(BlueprintCallable)
	inline void SetIsDashing(bool bIsDashing);

	UFUNCTION(BlueprintCallable)
	inline void SetIsFighting(bool IN_IsFighting);

	UFUNCTION(BlueprintCallable)
	inline void SetIsMoving(bool IN_IsMoving);

	UFUNCTION(BlueprintCallable)
	inline void SetIsSprinting(bool IN_IsSprinting);

	UFUNCTION(BlueprintCallable)
	inline void SetIsTakingDamage(bool IN_IsTakingDamage);

	UPROPERTY()
	FOnStateDirty OnDirtyDelegate;

protected:
	// TODO: Figure out why making all properties using OnRep fixes the bug with inconsistent updates for client
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty)
	bool IsDirty = true;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere, BlueprintReadOnly)
	bool IsDashing = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere, BlueprintReadOnly)
	bool IsDying = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere, BlueprintReadOnly)
	bool IsFighting = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere, BlueprintReadOnly)
	bool IsMoving = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere, BlueprintReadOnly)
	bool IsPicking = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere, BlueprintReadOnly)
	bool IsSprinting = false;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, VisibleAnywhere, BlueprintReadOnly)
	bool IsTakingDamage = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);

		DOREPLIFETIME(UMStateModelComponent, IsDirty);
		DOREPLIFETIME(UMStateModelComponent, IsDashing);
		DOREPLIFETIME(UMStateModelComponent, IsDying);
		DOREPLIFETIME(UMStateModelComponent, IsFighting);
		DOREPLIFETIME(UMStateModelComponent, IsMoving);
		DOREPLIFETIME(UMStateModelComponent, IsPicking);
		DOREPLIFETIME(UMStateModelComponent, IsSprinting);
		DOREPLIFETIME(UMStateModelComponent, IsTakingDamage);
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
