#pragma once

#include "MPickableActor.h"
#include "MDropActor.generated.h"

class UMDropManager;
//~=============================================================================
/**
 * Drop actor represented by the dropped item icon sprite. Can store only items of same ID.
 * Disappears after picking up all the contents(quantity). Base class: MPickableActor 
 */
UCLASS(Blueprintable)
class AMDropActor : public AMPickableActor
{
	GENERATED_BODY()

public:

	virtual void InitialiseInventory(const TArray<FItem>& IN_Items) override;
};

