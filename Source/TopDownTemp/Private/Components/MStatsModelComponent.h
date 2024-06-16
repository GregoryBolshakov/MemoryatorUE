#pragma once

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "MStatsModelComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatDirty);

USTRUCT(BlueprintType)
struct FClampedFloat
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float Value = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MinValue = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MaxValue = 0.f;
};

/** Component that stores and replicates all measurable stats e.g. Health, Strength, WalkSpeed etc.\n
 * Supposed to be attached to AMCharacter or AMActor */
UCLASS(BlueprintType)
class UMStatsModelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	bool GetIsDirty() const { return IsDirty; }
	void CleanDirty() { IsDirty = false; }

	UFUNCTION(BlueprintCallable)
	bool GetCanRetreat() const { return bCanRetreat; }

	/** Forces to add radius on purpose. Fight range doesn't make much sense on its own and Radius is very frequently forgotten. */
	UFUNCTION(BlueprintCallable)
	float GetFightRangePlusRadius(float Radius) const { return FightRange + Radius; }

	UFUNCTION(BlueprintCallable)
	float GetForgetEnemyRange() const { return ForgetEnemyRange; }

	UFUNCTION(BlueprintCallable)
	float GetHealth() const { return Health.Value; }

	UFUNCTION(BlueprintCallable)
	float GetMaxHealth() const { return Health.MaxValue; }

	UFUNCTION(BlueprintCallable)
	float GetMeleeSpread() const { return MeleeSpread; }

	UFUNCTION(BlueprintCallable)
	float GetRetreatRange() const { return RetreatRange; }

	UFUNCTION(BlueprintCallable)
	float GetSightRange() const { return SightRange; }

	UFUNCTION(BlueprintCallable)
	float GetSprintSpeed() const { return SprintSpeed; }
	
	UFUNCTION(BlueprintCallable)
	float GetSpeakingRange() const { return SpeakingRange; }

	UFUNCTION(BlueprintCallable)
	float GetStrength() const { return Strength; }

	UFUNCTION(BlueprintCallable)
	float GetTimeBeforeSprint() const { return TimeBeforeSprint; }

	UFUNCTION(BlueprintCallable)
	float GetWalkSpeed() const { return WalkSpeed; }

	UFUNCTION(BlueprintCallable)
	inline void SetCanRetreat(bool IN_CanRetreat);

	UFUNCTION(BlueprintCallable)
	inline void SetForgetEnemyRange(float IN_ForgetEnemyRange);

	UFUNCTION(BlueprintCallable)
	inline void SetHealth(float IN_Health);

	UFUNCTION(BlueprintCallable)
	inline void SetMeleeSpread(float IN_MeleeSpread);

	UFUNCTION(BlueprintCallable)
	inline void SetRetreatRange(float IN_RetreatRange);

	UFUNCTION(BlueprintCallable)
	inline void SetSightRange(float IN_SightRange);

	UFUNCTION(BlueprintCallable)
	inline void SetSpeakingRange(float IN_SpeakingRange);

	UFUNCTION(BlueprintCallable)
	inline void SetSprintSpeed(float IN_SprintSpeed);

	UFUNCTION(BlueprintCallable)
	inline void SetStrength(float IN_Strength);

	UFUNCTION(BlueprintCallable)
	inline void SetTimeBeforeSprint(float IN_TimeBeforeSprint);

	UFUNCTION(BlueprintCallable)
	inline void SetWalkSpeed(float IN_WalkSpeed);

	UPROPERTY()
	FOnStatDirty OnDirtyDelegate;

protected:
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty)
	bool IsDirty = true;

	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	bool bCanRetreat = true;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	float FightRange = 150.f;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	float ForgetEnemyRange = 2500.f;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	FClampedFloat Health = {100.f, 0.f, 100.f};
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	float MeleeSpread = 40.f;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	float RetreatRange = 500.f;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	float SightRange = 1500.f;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	float SpeakingRange = 300.f;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	float SprintSpeed = 650.f;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	float Strength = 20.f;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	float TimeBeforeSprint = 1.f;
	UPROPERTY(ReplicatedUsing=OnRep_IsDirty, EditDefaultsOnly, BlueprintReadWrite, Category = Stats)
	float WalkSpeed = 450.f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);

		DOREPLIFETIME(UMStatsModelComponent, bCanRetreat);
		DOREPLIFETIME(UMStatsModelComponent, FightRange);
		DOREPLIFETIME(UMStatsModelComponent, ForgetEnemyRange);
		DOREPLIFETIME(UMStatsModelComponent, Health);
		DOREPLIFETIME(UMStatsModelComponent, MeleeSpread);
		DOREPLIFETIME(UMStatsModelComponent, RetreatRange);
		DOREPLIFETIME(UMStatsModelComponent, SightRange);
		DOREPLIFETIME(UMStatsModelComponent, SpeakingRange);
		DOREPLIFETIME(UMStatsModelComponent, SprintSpeed);
		DOREPLIFETIME(UMStatsModelComponent, Strength);
		DOREPLIFETIME(UMStatsModelComponent, TimeBeforeSprint);
		DOREPLIFETIME(UMStatsModelComponent, WalkSpeed);
	}

	UFUNCTION()
	void OnRep_IsDirty()
	{
		OnDirtyDelegate.Broadcast();
	}
};

#if CPP
#include "MStatsModelComponent.inl"
#endif
