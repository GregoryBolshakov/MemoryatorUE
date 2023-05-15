#include "MCommunicationManager.h"

#include "MCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "MCommunicationWidget.h"
#include "MInventoryComponent.h"
#pragma optimize("", off)
AMCommunicationManager::AMCommunicationManager()
{
	InventoryToOffer = CreateDefaultSubobject<UMInventoryComponent>("InventoryToOffer");
	InventoryToReward = CreateDefaultSubobject<UMInventoryComponent>("InventoryToReward");

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

	// Remove the communication screen widget from previous conversation if it was present
	if (CommunicationWidget && IN_InterlocutorCharacter != InterlocutorCharacter)
	{
		CommunicationWidget->RemoveFromParent(); // Remove widget instantly on purpose, don't need the hide animation overlapping the opening animation
		CommunicationWidget = nullptr;
	}

	InterlocutorCharacter = IN_InterlocutorCharacter;
	GenerateInventoryToReward();
	InventoryToOffer->OnAnySlotChangedDelegate.AddLambda([this]
	{
		GenerateInventoryToReward();
		if (CommunicationWidget)
		{
			CommunicationWidget->ReCreateRewardItemSlotWidgets();
		}
	});

	// Create the communication screen widget
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
	ReturnAllPlayerItems();
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

void AMCommunicationManager::GenerateInventoryToReward()
{
	if (!InterlocutorCharacter) return;
	InventoryToReward->Initialize(1, {{2, 3}});
	InventoryToReward->LockAllItems();
}

void AMCommunicationManager::ReturnAllPlayerItems()
{
	if (const auto World = GetWorld())
	{
		if (const auto PlayerCharacter = Cast<AMCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)))
		{
			if (const auto PlayerInventory = PlayerCharacter->GetInventoryComponent())
			{
				// Return all the items to the player inventory. If doesn't fit, spawn as a drop
				for (auto& ItemSlot : InventoryToOffer->GetSlots())
				{
					PlayerInventory->StoreItem(ItemSlot.Item);
					ItemSlot.Item = {0, 0};
				}
			}
		}
	}
}
#pragma optimize("", on)
