#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MItemsDataAsset.h"
#include "MGameInstance.generated.h"

UCLASS()
class TOPDOWNTEMP_API UMGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MyDataAsset")
	UMItemsDataAsset* ItemsDataAsset;
};