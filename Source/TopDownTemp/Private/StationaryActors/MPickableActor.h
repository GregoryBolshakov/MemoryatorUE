#pragma once

#include "CoreMinimal.h"
#include "MActor.h"
#include "MPickableActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickedUpCompletely, TSubclassOf<AMPickableActor>, Class);

class UMDropManager;
//~=============================================================================
/**
 * MActor with Inventory Component. Player can pick up/pull out items from it.
 * This might be some drop on any container.
 */
UCLASS(Blueprintable)
class AMPickableActor : public AMActor
{
	GENERATED_UCLASS_BODY()

public:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnItemChanged(int NewItemID, int NewQuantity);

	UPROPERTY(BlueprintAssignable)
	FOnPickedUpCompletely PickedUpCompletelyDelegate;

protected:

	virtual void PostInitializeComponents() override;

	//TODO: Consider making player in charge of tracking overlaps, it improves performance, but limits possible player character switches
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	UPROPERTY(EditDefaultsOnly, Category = AMPickableItem)
	bool bDisappearIfEmptyInventory;
};
