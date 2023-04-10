#pragma once

#include "CoreMinimal.h"
#include "MActor.h"
#include "MInventoryComponent.h"
#include "MPickableItem.generated.h"

class UMDropManager;
//~=============================================================================
/**
 * Item that can be picked up
 */
UCLASS(Blueprintable)
class AMPickableItem : public AMActor //For now is useless
{
	GENERATED_UCLASS_BODY()

public:

	void Initialise(const FItem& IN_Item);

	UFUNCTION()
	void OnItemChanged(int NewItemID, int NewQuantity);

protected:

	virtual void PostInitializeComponents() override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AMPickableItem, meta = (AllowPrivateAccess = "true"))
	UMInventoryComponent* InventoryComponent;

	/** A pointer for easier access of World Generator's Drop Manager */
	UPROPERTY()
	UMDropManager* pDropManager;
};

