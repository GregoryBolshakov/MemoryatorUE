#pragma once

#include "CoreMinimal.h"
#include "MCharacterSpeciesDataAsset.h"
#include "MCommunicationComponent.generated.h"

class UMPriceCoefficientsSet;

UCLASS(BlueprintType)
class TOPDOWNTEMP_API UMCommunicationComponent : public UActorComponent
{
	GENERATED_BODY()

protected:

	/** Any character can adjust the coefficients and is not obliged to use only the default ones for their type.
	 * this is due to their ability to improve or reflect their attitude towards another character (negative or positive, trusting or fearful)
	 */
	FPriceCoefficientsSet PriceCoefficientsSet;
};