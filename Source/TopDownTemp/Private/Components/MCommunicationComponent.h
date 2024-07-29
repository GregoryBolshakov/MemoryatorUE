#pragma once

#include "CoreMinimal.h"
#include "DataAssets/MCharacterSpeciesDataAsset.h"
#include "MCommunicationComponent.generated.h"

class UMPriceCoefficientsSet;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnInterlocutorChanged, AMCharacter* Interlocutor);

UCLASS(BlueprintType)
class TOPDOWNTEMP_API UMCommunicationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	const AMCharacter* GetInterlocutorCharacter() const { return InterlocutorCharacter; }

	FORCEINLINE void SetInterlocutorCharacter(AMCharacter* Interlocutor);

	void StopSpeaking();

	FOnInterlocutorChanged OnInterlocutorChangedDelegate;

protected:

	/** Any character can adjust the coefficients and is not obliged to use only the default ones for their type.
	 * this is due to their ability to improve or reflect their attitude towards another character (negative or positive, trusting or fearful)
	 */
	FPriceCoefficientsSet PriceCoefficientsSet;

	UPROPERTY()
	AMCharacter* InterlocutorCharacter;
};