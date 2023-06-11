#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "MBuffBarComponent.generated.h"

class UMBuffBarWidget;
UENUM(BlueprintType)
enum class EBuffType : uint8
{
	Invulnerability = 0,
	KnockBackImmune,
	Poison,
	Fire
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnBuffFinished, int, Stack);

USTRUCT()
struct FBuff
{
	GENERATED_BODY()

	int Stack = 0;

	FTimerHandle TimerHandle;
};

UCLASS(BlueprintType)
class TOPDOWNTEMP_API UMBuffBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

	UMBuffBarComponent();

public:

	void AddBuff(EBuffType Type, float Duration);

	void RemoveBuff(EBuffType Type);

	bool IsBuffSet(EBuffType Type);

	void CreateWidget();

	TMap<EBuffType, FOnBuffFinished> BuffDelegates;

protected:

	void ValidateTimers(); // TODO: Implement

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void BeginPlay() override;

	TMap<EBuffType, FBuff> ActiveBuffs;
};