#include "MCommunicationManager.h"

#include "Characters/MCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "UI/MCommunicationWidget.h"
#include "Components/MInventoryComponent.h"
#include "Components/MStatsModelComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Components/MCommunicationComponent.h"
#include "Components/MStateModelComponent.h"

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

	const float SpeakingRange = PlayerCharacter->GetStatsModelComponent()->GetSpeakingRange();
	if (FVector::Dist(PlayerCharacter->GetActorLocation(), IN_InterlocutorCharacter->GetActorLocation()) > SpeakingRange)
		return;

	// Remove the communication screen widget from previous conversation if it was present
	if (CommunicationWidget && IN_InterlocutorCharacter != InterlocutorCharacter)
	{
		CommunicationWidget->RemoveFromParent(); // Remove widget instantly on purpose, don't need the hide animation overlapping the opening animation
		CommunicationWidget = nullptr;
	}

	// TODO: Refactor using only CommunicationComponent
	InterlocutorCharacter = IN_InterlocutorCharacter;
	InterlocutorCharacter->GetCommunicationComponent()->SetInterlocutorCharacter(PlayerCharacter);
	PlayerCharacter->GetStateModelComponent()->SetIsCommunicating(true);
	InterlocutorCharacter->GetStateModelComponent()->SetIsCommunicating(true);
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

	// TODO: Refactor using only CommunicationComponent
	if (InterlocutorCharacter)
	{
		InterlocutorCharacter->GetCommunicationComponent()->SetInterlocutorCharacter(nullptr);
		InterlocutorCharacter->GetStateModelComponent()->SetIsCommunicating(false);
		InterlocutorCharacter = nullptr;
	}

	if (const auto PlayerCharacter = Cast<AMCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		PlayerCharacter->GetStateModelComponent()->SetIsCommunicating(false);
	}

	ReturnAllPlayerItems();
}

namespace
{
	ETeamAttitude::Type CustomTeamAttitudeSolver(FGenericTeamId A, FGenericTeamId B)
	{
		// Normalize the order of A and B to ensure consistency
		if (A.GetId() > B.GetId())
		{
			Swap(A, B);
		}

		if (A == GetTeamIdByEnum(EMTeamID::Nightmares) && B == GetTeamIdByEnum(EMTeamID::Witches))
		{
			return ETeamAttitude::Neutral;
		}

		// TODO: Put other cross-faction attitudes here

		return A != B ? ETeamAttitude::Hostile : ETeamAttitude::Friendly;
	}
}

void AMCommunicationManager::BeginPlay()
{
	Super::BeginPlay();

	FGenericTeamId::SetAttitudeSolver(CustomTeamAttitudeSolver);

	InventoryToOffer->Initialize(8, {});
}

void AMCommunicationManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Check if the player had had a conversation with some mob but ran away
	if (InterlocutorCharacter)
	{
		if (const auto PlayerCharacter = Cast<AMCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
		{
			const float SpeakingRange = PlayerCharacter->GetStatsModelComponent()->GetSpeakingRange();
			if (FVector::Dist(PlayerCharacter->GetActorLocation(), InterlocutorCharacter->GetActorLocation()) > SpeakingRange)
			{
				StopSpeaking();
			}
		}
	}
}

void AMCommunicationManager::GenerateInventoryToReward()
{
	InventoryToReward->Empty();
	InventoryToReward->Initialize(0, {});

	if (!InterlocutorCharacter) return;

	if (InventoryToOffer->GetSlots().IsEmpty())
	{
		//TODO: Check if quest is complete and offer some reward, etc. 
	}
	else
	{
		// Trade:
		//TODO: Come up with a text that the mob says
		if (InterlocutorCharacter)
		{
			if (const auto Inventory = InterlocutorCharacter->GetInventoryComponent())
			{
				// The items the interlocutor offers for the 
				auto CounterOfferItems = Inventory->MaxPriceCombination(UMInventoryComponent::GetTotallPrice(InventoryToOffer->GetSlots(), GetWorld()));
				UMInventoryComponent::StackItems(CounterOfferItems);
				UMInventoryComponent::SortItems(CounterOfferItems, GetWorld());
				auto OfferItemCopies = InventoryToOffer->GetItemCopies();
				UMInventoryComponent::StackItems(OfferItemCopies);
				UMInventoryComponent::SortItems(OfferItemCopies, GetWorld());
				if (CounterOfferItems != OfferItemCopies) // TODO: IMPORTANT! Implement randomize in MaxPriceCombination to avoid this case
				{
					InventoryToReward->Initialize(CounterOfferItems.Num(), CounterOfferItems); // TODO: Not initialize, but append. There might be some free reward for quest
					InventoryToReward->SetFlagToAllSlots(FSlot::ESlotFlags::PreviewOnly);
				}
			}
		}
	}
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
					if (ItemSlot.Item.Quantity <= 0)
						continue;
					PlayerInventory->StoreItem(ItemSlot.Item);
					ItemSlot.Item = {0, 0};
				}
			}
		}
	}
}

void AMCommunicationManager::MakeADeal()
{
	auto InterlocutorInventory = InterlocutorCharacter->GetInventoryComponent();
	if (!InterlocutorInventory) { check(false); return; }

	// Player takes all unlocked items from the reward inventory
	if (const auto World = GetWorld())
	{
		if (const auto PlayerCharacter = Cast<AMCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)))
		{
			if (const auto PlayerInventory = PlayerCharacter->GetInventoryComponent())
			{
				for (const auto& Slot : InventoryToReward->GetSlots())
				{
					InterlocutorInventory->RemoveItem(Slot.Item);
					PlayerInventory->StoreItem(Slot.Item);
				}
			}
		}
	}

	InventoryToReward->Empty();

	// Interlocutor takes all items from the offer inventory
	for (const auto& Slot : InventoryToOffer->GetSlots())
	{
		if (Slot.Item.Quantity == 0)
			continue;
		if (!InterlocutorInventory->IsEnoughSpace(Slot.Item, GetWorld()))
			break;
		InterlocutorInventory->StoreItem(Slot.Item);
		// The current implementation is such that if the item doesn't fit, it is deleted and no longer available.
		// It is no longer in their inventory. We assume the interlocutor hid it somewhere.
	}

	InventoryToOffer->Empty();
}
