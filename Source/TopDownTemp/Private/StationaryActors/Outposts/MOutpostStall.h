#pragma once

#include "CoreMinimal.h"
#include "StationaryActors/Outposts/MOutpostElement.h"
#include "MOutpostStall.generated.h"

class AMCharacter;
class AMOutpostGenerator;

//~=============================================================================
/**
 * Base class for a stall that is part of an outpost.\n It can be any element where a resident can trade
 */
UCLASS(Blueprintable)
class AMOutpostStall : public AMOutpostElement
{
	GENERATED_BODY()

public:
	AMCharacter* GetTrader() const { return Trader; }
	void SetTrader(AMCharacter* NewTrader) { Trader = NewTrader; }

	UFUNCTION()
	void ResetTrader();

	/** Returns the location where the trader should be. */
	UFUNCTION(BlueprintCallable)
	FVector GetEntryPoint() const; // TODO: The same as for AMOutpostHouse, may create a common base class for them

protected:

	/** The trader. So far, we support only one trader per stall. nullptr if not occupied. */
	UPROPERTY()
	AMCharacter* Trader;

	// TODO: Support specialization requirement (blacksmith, baker, farmer, etc.)
};
