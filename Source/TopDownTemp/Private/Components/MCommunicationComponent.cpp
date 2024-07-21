#include "MCommunicationComponent.h"

#include "Framework/MGameMode.h"

void UMCommunicationComponent::StopSpeaking()
{
	//AMGameMode::GetCommunicationManager(this)->OnStopSpeaking();

	InterlocutorCharacter = nullptr;
}
