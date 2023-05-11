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

	UMInventoryComponent* GetInventory() const { return InventoryToOffer; }

protected:

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY()
	AMCharacter* InterlocutorCharacter;

	UPROPERTY()
	class UMCommunicationWidget* CommunicationWidget;

	UPROPERTY(EditDefaultsOnly, Category = MCommunicationManager)
	TSubclassOf<UMCommunicationWidget> CommunicationWidgetBPClass;

	UPROPERTY(EditDefaultsOnly, Category = MCommunicationManager)
	float CommunicationDistance = 100.f;

	/** This is a temporary inventory with empty slots player uses to offer their items to an interlocutor */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = MCommunicationManager)
	UMInventoryComponent* InventoryToOffer;
};

