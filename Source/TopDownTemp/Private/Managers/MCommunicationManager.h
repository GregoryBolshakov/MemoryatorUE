#pragma once

#include "CoreMinimal.h"
#include "MCommunicationManager.generated.h"

class UMInventoryComponent;
class AMCharacter;
class UMCommunicationWidget;
class FSocket;

UCLASS(Blueprintable, BlueprintType)
class AMCommunicationManager : public AActor
{
	GENERATED_BODY()

public:
	void ConnectToPythonServer();

	bool IsConnected() const { return Socket != nullptr && Connected; }

	void SendMessagesToServer(const AMCharacter* Character);

protected:
	void ReadDataFromSocket();

	/** When messages stack up, remove all previous ones. For example:
	 * Ah, those<end>
	 * Ah, those<end>Ah, those sound lovely<end>
	 * Ah, those<end>Ah, those sound lovely<end>Ah, those sound lovely indeed!<end>
	 * Should be treated simply as "Ah, those sound lovely indeed!<end>" */
	static void CutPreviousMessages(FString& Message);

	/**
	 * 
	 * @param Character who will be saying the generated response
	 * @return json struct to send to the server
	 */
	FString GenerateMessagesJson(const AMCharacter* Character);

	virtual void BeginPlay() override;

	FSocket* Socket;

	bool Connected = false;
};

