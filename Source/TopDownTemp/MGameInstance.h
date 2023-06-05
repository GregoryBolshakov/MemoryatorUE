#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MItemsDataAsset.h"
#include "MCharacterSpeciesDataAsset.h"
#include "MGameInstance.generated.h"

UCLASS()
class TOPDOWNTEMP_API UMGameInstance : public UGameInstance
{
	GENERATED_UCLASS_BODY()

public:

	//TODO: Put these to the game mode
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MGameInstance")
	UMItemsDataAsset* ItemsDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MGameInstance")
	UMCharacterSpeciesDataAsset* CharacterSpeciesDataAsset;

private:

	class FNakamaManagerModule* NakamaManagerModule;
};