#pragma once

#include "CoreMinimal.h"
#include "MCommunicationManager.generated.h"

class UMInventoryComponent;
class AMCharacter;
class UMCommunicationWidget;

UCLASS(Blueprintable, BlueprintType)
class AMCommunicationManager : public AActor
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;
};

