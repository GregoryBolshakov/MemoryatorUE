#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DataAssets/MItemsDataAsset.h"
#include "DataAssets/MCharacterSpeciesDataAsset.h"
#include "MGameInstance.generated.h"

class UNakamaManager;

UCLASS()
class TOPDOWNTEMP_API UMGameInstance : public UGameInstance
{
	GENERATED_UCLASS_BODY()

public:

	UNakamaManager* GetNakamaManager();

	//TODO: Put these to the game mode
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MGameInstance")
	UMItemsDataAsset* ItemsDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MGameInstance")
	UMCharacterSpeciesDataAsset* CharacterSpeciesDataAsset;

private:

	class FNakamaManagerModule* NakamaManagerModule;
};