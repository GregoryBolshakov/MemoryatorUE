#pragma once

#include "CoreMinimal.h"
#include "MCommunicationManager.generated.h"

class UMInventoryComponent;
class AMCharacter;

//TODO: Consider implementing conversations between mobs as well
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

	UMInventoryComponent* GetInventoryToOffer() const { return InventoryToOffer; }

	UMInventoryComponent* GetInventoryToReward() const { return InventoryToReward; }

protected:

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	void GenerateInventoryToReward();

	void ReturnAllPlayerItems();

	UPROPERTY()
	AMCharacter* InterlocutorCharacter;

	UPROPERTY()
	class UMCommunicationWidget* CommunicationWidget;

	UPROPERTY(EditDefaultsOnly, Category = MCommunicationManager)
	TSubclassOf<UMCommunicationWidget> CommunicationWidgetBPClass;

	UPROPERTY(EditDefaultsOnly, Category = MCommunicationManager)
	float CommunicationDistance = 100.f;

	/** Temporary inventory with empty slots player uses to offer their items to an interlocutor */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = MCommunicationManager)
	UMInventoryComponent* InventoryToOffer;

	/** Temporary inventory. Source of reward items which don't belong to a mob but can be given out */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = MCommunicationManager)
	UMInventoryComponent* InventoryToReward;
};

