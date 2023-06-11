// Copyright Epic Games, Inc. All Rights Reserved.

#include "MGameMode.h"
#include "Controllers/MPlayerController.h"
#include "Characters/MCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMGameMode::AMGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = AMPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Memoryator/Blueprints/BP_Memoryator"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}