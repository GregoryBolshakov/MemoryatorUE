#include "MCommunicationComponent.h"

#include "Framework/MGameMode.h"

void UMCommunicationComponent::SetInterlocutorCharacter(AMCharacter* Interlocutor)
{
	InterlocutorCharacter = Interlocutor;
	OnInterlocutorChangedDelegate.Broadcast(InterlocutorCharacter);
}

void UMCommunicationComponent::StopSpeaking()
{
	//AMGameMode::GetCommunicationManager(this)->OnStopSpeaking();

	InterlocutorCharacter = nullptr;
}
