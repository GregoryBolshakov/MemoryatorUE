#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MCharacterSpeciesDataAsset.generated.h"

/** The list of basic creature types */
UENUM(BlueprintType)
enum class ECreatureType : uint8
{
	Bogatyr = 0,
	Villager,
	Nightmare,
	Animal
};

USTRUCT(BlueprintType)
struct TOPDOWNTEMP_API FPriceCoefficientsSet
{
	GENERATED_BODY()

	/** The coefficients by which the owner multiplies the price of an item when selling.
	 * Because most likely they don't know the real price or is specially greedy/foolish
	 * Key is the item ID. Value is the coefficient */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<int, float> PriceCoefficientsToSell;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int DefaultCoefficientToSell = 1.2f;

	/** The coefficients by which the owner multiplies the price of an item when buying.
	 * Because most likely they don't know the real price or is specially greedy/foolish
	 * Key is the item ID. Value is the coefficient */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<int, float> PriceCoefficientsToBuy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int DefaultCoefficientToBuy = 0.8f;
};

USTRUCT(BlueprintType)
struct TOPDOWNTEMP_API FStartingItemData
{
	GENERATED_BODY()

	/** 0 to 1 probability of possessing this item at the begin play */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Probability;

	/** If, after a probability check the item appears, this determines the bounds of the quantity */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FIntPoint MinMaxStartQuantity;
};

/** Data of one particular species */
USTRUCT(BlueprintType)
struct TOPDOWNTEMP_API FCharacterData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECreatureType CreatureType;

	/** The mapping between character's species name and the default coefficients by which they multiply item prices
	 *  See UMCharacterSpeciesDataAsset.h for better understanding */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FPriceCoefficientsSet PriceCoefficientsSet;

	/** Determines the probabilities of having specific items and their quantities at the begin play*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<int, FStartingItemData> StartingItemsData; 
};

UCLASS(BlueprintType)
class TOPDOWNTEMP_API UMCharacterSpeciesDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	/** The list of all possible existing character species in the game */
	UPROPERTY(Category=MCharacterSpeciesDataAsset, EditAnywhere, BlueprintReadOnly)
	TMap<FName, FCharacterData> Data;
};
