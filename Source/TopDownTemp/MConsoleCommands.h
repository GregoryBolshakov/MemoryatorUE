#pragma once

#include "CoreMinimal.h"
#include "MConsoleCommands.generated.h"

//~=============================================================================
/**
 *  The class which is extended in blueprints to hold the commands.
 *  It is an UActorComponent so that it can automatically replicate when attached to Actors
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class UMConsoleCommands : public UActorComponent
{
	GENERATED_BODY()

public:
	UMConsoleCommands();
};

