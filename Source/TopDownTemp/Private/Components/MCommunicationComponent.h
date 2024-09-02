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
	UMCommunicationComponent();

	const AMCharacter* GetInterlocutorCharacter() const { return InterlocutorCharacter; }

	FORCEINLINE void SetInterlocutorCharacter(AMCharacter* Interlocutor);

	UFUNCTION(Server, Reliable)
	void Server_TalkToCharacter(AMCharacter* IN_InterlocutorCharacter);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_StopTalking();

	void GenerateInventoryToReward(const UMInventoryComponent* InventoryToOffer, UMInventoryComponent* InventoryToReward) const;

	/** Take all unlocked items from the reward inventory. Give all items from the offer inventory */
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_MakeADeal(UMInventoryComponent* InventoryToOffer, UMInventoryComponent* InventoryToReward);

	/** Returns all items in InventoryToOffer back to player's inventory without making any deal. */
	UFUNCTION(Server, Reliable)
	void Server_CancelOffer(UMInventoryComponent* InventoryToOffer) const;

	FOnInterlocutorChanged OnInterlocutorChangedDelegate;

protected:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Any character can adjust the coefficients and is not obliged to use only the default ones for their type.
	 * this is due to their ability to improve or reflect their attitude towards another character (negative or positive, trusting or fearful)
	 */
	FPriceCoefficientsSet PriceCoefficientsSet;

	UPROPERTY(Replicated)
	AMCharacter* InterlocutorCharacter = nullptr;
};