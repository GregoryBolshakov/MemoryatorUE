#pragma once

#include "CoreMinimal.h"
#include "MConsoleCommands.h"
#include "MConsoleCommandsManager.generated.h"

//~=============================================================================
/**
 *  The class which lives on the PlayerController.
 *  It is responsible for spawning UConsoleCommands objects and routing the commands to these objects.
 */
UCLASS(Within = PlayerController, BlueprintType)
class UMConsoleCommandsManager : public UActorComponent
{
	GENERATED_BODY()

public:

	UMConsoleCommandsManager();

	virtual void BeginPlay() override;

	virtual bool ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor) override;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<UMConsoleCommands>> ConsoleCommandsClasses;

private:

	UPROPERTY(Transient, Replicated)
	TArray<UMConsoleCommands*> ConsoleCommandsObjects;
};

