#pragma once

#include "CoreMinimal.h"
#include "MActor.h"
#include "Components/MInventoryComponent.h"
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

	UFUNCTION(BlueprintCallable)
	virtual void InitialiseInventory(const TArray<FItem>& IN_Items);

	UFUNCTION()
	void OnItemChanged(int NewItemID, int NewQuantity);

	UPROPERTY(BlueprintAssignable)
	FOnPickedUpCompletely PickedUpCompletelyDelegate;

protected:

	virtual void PostInitializeComponents() override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	UPROPERTY(EditDefaultsOnly, Category = AMPickableItem)
	bool bDisappearIfEmptyInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AMPickableItem, meta = (AllowPrivateAccess = "true"))
	UMInventoryComponent* InventoryComponent;

	/** A pointer for easier access of World Generator's Drop Manager */
	UPROPERTY()
	UMDropManager* pDropManager;
};

