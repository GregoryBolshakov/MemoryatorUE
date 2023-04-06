#pragma once

#include "CoreMinimal.h"
#include "MActor.h"
#include "MInventoryComponent.h"
#include "MPickableItem.generated.h"

//~=============================================================================
/**
 * Item that can be picked up
 */
UCLASS(Blueprintable)
class AMPickableItem : public AMActor //For now is useless
{
	GENERATED_UCLASS_BODY()

public:

	virtual void PostInitializeComponents() override;

	void SetItem(const FItem& IN_Item);

protected:

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AMPickableItem, meta=(AllowPrivateAccess=true))
	FItem Item;
};

