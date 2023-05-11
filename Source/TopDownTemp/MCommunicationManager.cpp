#include "MCommunicationManager.h"

#include "MCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "MCommunicationWidget.h"
#include "MInventoryComponent.h"

AMCommunicationManager::AMCommunicationManager()
{
	InventoryToOffer = CreateDefaultSubobject<UMInventoryComponent>("InventoryToOffer");

	//TODO: Either turn tick off when not speaking or disable the whole actor with MIsActiveCheckerComponent
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AMCommunicationManager::SpeakTo(AMCharacter* IN_InterlocutorCharacter)
{
	const auto PlayerCharacter = Cast<AMCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!IsValid(PlayerCharacter) || !IsValid(IN_InterlocutorCharacter)) { check(false); return; }

	if (FVector::Dist(PlayerCharacter->GetActorLocation(), IN_InterlocutorCharacter->GetActorLocation()) > 100.f) // TODO: put to variables
		return;

	if (CommunicationWidget && IN_InterlocutorCharacter != InterlocutorCharacter)
	{
		CommunicationWidget->RemoveFromParent(); // Remove widget instantly on purpose, don't need hide animation overlapping open animation
		CommunicationWidget = nullptr;
	}

	InterlocutorCharacter = IN_InterlocutorCharacter;

	if (!CommunicationWidget)
	{
		CommunicationWidget = CreateWidget<UMCommunicationWidget>(GetWorld()->GetFirstPlayerController(), CommunicationWidgetBPClass);
		CommunicationWidget->AddToPlayerScreen();
	}
}

void AMCommunicationManager::StopSpeaking()
{
	if (CommunicationWidget)
	{
		CommunicationWidget->Close();
		CommunicationWidget = nullptr;
	}
	InterlocutorCharacter = nullptr;
}

void AMCommunicationManager::BeginPlay()
{
	Super::BeginPlay();

	InventoryToOffer->Initialize(8, {});
}

void AMCommunicationManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Check if the player had had a conversation with some mob but ran away
	if (InterlocutorCharacter)
	{
		if (const auto PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
		{
			if (FVector::Dist(PlayerCharacter->GetActorLocation(), InterlocutorCharacter->GetActorLocation()) > CommunicationDistance)
			{
				StopSpeaking();
			}
		}
	}
}
