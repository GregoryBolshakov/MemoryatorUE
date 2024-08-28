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

	void SpeakTo(AMCharacter* IN_InterlocutorCharacter);

	UFUNCTION(BlueprintCallable)
	void StopSpeaking();

	void GenerateInventoryToReward(const UMInventoryComponent* InventoryToOffer, UMInventoryComponent* InventoryToReward) const;

	/** Take all unlocked items from the reward inventory. Give all items from the offer inventory */
	UFUNCTION(BlueprintCallable)
	void MakeADeal(UMInventoryComponent* InventoryToOffer, UMInventoryComponent* InventoryToReward);

	// TODO: Use additional parameter for InventoryToReturn or something
	/** Returns all items in InventoryToOffer back to player's inventory without making any deal. */
	void CancelOffer(UMInventoryComponent* InventoryToOffer) const;

	FOnInterlocutorChanged OnInterlocutorChangedDelegate;

protected:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Any character can adjust the coefficients and is not obliged to use only the default ones for their type.
	 * this is due to their ability to improve or reflect their attitude towards another character (negative or positive, trusting or fearful)
	 */
	FPriceCoefficientsSet PriceCoefficientsSet;

	UPROPERTY()
	AMCharacter* InterlocutorCharacter;
};