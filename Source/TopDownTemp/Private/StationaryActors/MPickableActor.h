#pragma once

#include "CoreMinimal.h"
#include "MActor.h"
#include "Components/MInventoryComponent.h"
#include "MPickableActor.generated.h"

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

	virtual void InitialiseInventory(const TArray<FItem>& IN_Items);

	UFUNCTION()
	void OnItemChanged(int NewItemID, int NewQuantity);

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

