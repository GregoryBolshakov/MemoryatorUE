#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MGameMode.generated.h"

UCLASS(minimalapi)
class AMGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMGameMode();

	/** Temporarily switch to offline mode, continue the game and keep checking the connection */
	UFUNCTION(BlueprintCallable)
	void GoOffline();
};



