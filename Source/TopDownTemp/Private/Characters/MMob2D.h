#pragma once

#include "CoreMinimal.h"
#include "MCharacter2D.h"
#include "MMob2D.generated.h"

//TODO: Delete this class. It's a temp workaround
//~=============================================================================
/**
 * NPC that uses M2DRepresentationComponent, fully driven by AMMobController
 */
UCLASS(Blueprintable)
class AMMob2D : public AMCharacter2D
{
	GENERATED_UCLASS_BODY()

public:

	float GetPileInLength() const { return PileInLength; }

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=MobPerks)
	float PileInLength = 0.f;

};

