#pragma once

#include "CoreMinimal.h"
#include "MBuffManagerComponent.generated.h"

class UWidgetComponent;
class UMBuffBarWidget;
UENUM(BlueprintType)
enum class EBuffType : uint8
{
	Invulnerability = 0,
	KnockBackImmune,
	Poison,
	Fire
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnBuffEffect, int, Stack);

USTRUCT()
struct FBuff
{
	GENERATED_BODY()

	int Stack = 0;

	FTimerHandle TimerHandle;
};

UCLASS(BlueprintType)
class TOPDOWNTEMP_API UMBuffManagerComponent : public UActorComponent
{
	GENERATED_BODY()

	UMBuffManagerComponent();

public:

	void AddBuff(EBuffType Type, float Duration);

	bool IsBuffSet(EBuffType Type);

	void CreateWidget();

	TMap<EBuffType, FOnBuffEffect> BuffDelegates;

protected:

	void ValidateTimers(); // TODO: Implement

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	TMap<EBuffType, FBuff> ActiveBuffs;

	UPROPERTY()
	UWidgetComponent* BuffBarWidgetComponent;

	//TODO: create a separate entity to store this. Now it needs to be set for each ancestor (it's bad).
	UPROPERTY(EditDefaultsOnly, Category=MBuffManagerComponent)
	TSubclassOf<UMBuffBarWidget> BuffBarWidgetBPClass;
};