// Copyright Epic Games, Inc. All Rights Reserved.

#include "MConsoleCommandsManager.h"
#include "Net/UnrealNetwork.h"

UMConsoleCommandsManager::UMConsoleCommandsManager()
{
	SetIsReplicatedByDefault(true);
}

void UMConsoleCommandsManager::BeginPlay()
{
	Super::BeginPlay();

	// We only want to spawn the objects on the server, so they replicate down to the client.
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	for (const TSubclassOf<UMConsoleCommands>& Class : ConsoleCommandsClasses)
	{
		if (Class != nullptr)
		{
			ConsoleCommandsObjects.Add(NewObject<UMConsoleCommands>(GetOwner(), Class));
		}
	}
}

bool UMConsoleCommandsManager::ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor)
{
	for (UMConsoleCommands* Object : ConsoleCommandsObjects)
	{
		if (Object->ProcessConsoleExec(Cmd, Ar, Executor))
		{
			return true;
		}
	}

	return Super::ProcessConsoleExec(Cmd, Ar, Executor);
}

void UMConsoleCommandsManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMConsoleCommandsManager, ConsoleCommandsObjects);
}