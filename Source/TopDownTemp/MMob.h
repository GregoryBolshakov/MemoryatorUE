#pragma once

#include "CoreMinimal.h"
#include "MCharacter.h"
#include "MMob.generated.h"

//~=============================================================================
/**
 * NPC, fully driven by AMMobController
 */
UCLASS(Blueprintable)
class AMMob : public AMCharacter
{
	GENERATED_UCLASS_BODY()

public:

private:

	virtual void HandleAnimationStates() override;
};

