#pragma once

#include "CoreMinimal.h"
#include "Components/MInventoryComponent.h"
#include "MCommunicationManager.generated.h"

class UMInventoryComponent;
class AMCharacter;
class UMCommunicationWidget;

//TODO: Consider implementing conversations between mobs as well. If is doesn't make any sense, make this UObject
UCLASS(Blueprintable, BlueprintType)
class AMCommunicationManager : public AActor
{
	GENERATED_BODY()
	AMCommunicationManager();

public:

	void SpeakTo(AMCharacter* IN_InterlocutorCharacter);

	UFUNCTION(BlueprintCallable)
	void StopSpeaking();

	AMCharacter* GetInterlocutorCharacter() const { return InterlocutorCharacter; }

	/** Take all unlocked items from the reward inventory. Give all items from the offer inventory */
	UFUNCTION(BlueprintCallable)
	void MakeADeal(UMInventoryComponent* InventoryToOffer, UMInventoryComponent* InventoryToReward);

protected:

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	void GenerateInventoryToReward(const UMInventoryComponent* InventoryToOffer, UMInventoryComponent* InventoryToReward);

	/** Returns all items in InventoryToOffer back to player's inventory without making any deal. */
	void CancelOffer(UMInventoryComponent* InventoryToOffer) const;

	UPROPERTY()
	AMCharacter* InterlocutorCharacter;
};

