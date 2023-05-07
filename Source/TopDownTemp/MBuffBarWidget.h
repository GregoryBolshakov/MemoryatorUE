#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MBuffBarWidget.generated.h"

UCLASS()
class TOPDOWNTEMP_API UMBuffBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent)
	void AddBuff(EBuffType Type);

	UFUNCTION(BlueprintImplementableEvent)
	void RemoveBuff(EBuffType Type);

	UFUNCTION(BlueprintImplementableEvent)
	void SetCharacter(class AMCharacter* IN_Character);

protected:

	UPROPERTY(BlueprintReadWrite)
	TMap<EBuffType, class UImage*> BuffImages;
};