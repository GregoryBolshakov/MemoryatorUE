#pragma once

#include "MConsoleCommands.h"
#include "MConsoleCommandsWorld.generated.h"

//~=============================================================================
/**
 *  
 */
UCLASS(BlueprintType)
class UMConsoleCommandsWorld : public UMConsoleCommands
{
	GENERATED_BODY()

public:
	UMConsoleCommandsWorld();

	UFUNCTION(Exec)
	void SpawnMob(const FString& MobClassString, int Quantity = 1);
};

