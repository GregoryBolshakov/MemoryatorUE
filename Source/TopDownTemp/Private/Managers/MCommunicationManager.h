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

public:
	void ConnectToPythonServer();

protected:
	void ReadDataFromSocket(class FSocket* Socket);

	/** When messages stack up, remove all previous ones. For example:
	 * Ah, those<end>
	 * Ah, those<end>Ah, those sound lovely<end>
	 * Ah, those<end>Ah, those sound lovely<end>Ah, those sound lovely indeed!<end>
	 * Should be treated simply as "Ah, those sound lovely indeed!<end>" */
	static void CutPreviousMessages(FString& Message);

	virtual void BeginPlay() override;
};

