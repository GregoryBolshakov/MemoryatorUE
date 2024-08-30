#include "MCommunicationComponent.h"

#include "MStateModelComponent.h"
#include "MStatsModelComponent.h"
#include "Controllers/MInventoryControllerComponent.h"
#include "Controllers/MPlayerController.h"
#include "Framework/MGameMode.h"

UMCommunicationComponent::UMCommunicationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMCommunicationComponent::SetInterlocutorCharacter(AMCharacter* Interlocutor)
{
	if (auto* MCharacter = Cast<AMCharacter>(GetOwner()))
	{
		if (auto* StateModel = MCharacter->GetStateModelComponent())
		{
			StateModel->SetIsCommunicating(Interlocutor ? true : false);
			InterlocutorCharacter = Interlocutor;
			OnInterlocutorChangedDelegate.Broadcast(InterlocutorCharacter);
		}
	}
}

void UMCommunicationComponent::SpeakTo(AMCharacter* IN_InterlocutorCharacter)
{
	const auto OwnerCharacter = Cast<AMCharacter>(GetOwner());
	if (!IsValid(OwnerCharacter) || !IsValid(IN_InterlocutorCharacter)) { check(false); return; }

	const float SpeakingRange = OwnerCharacter->GetStatsModelComponent()->GetSpeakingRange();
	if (FVector::Dist(OwnerCharacter->GetActorLocation(), IN_InterlocutorCharacter->GetActorLocation()) > SpeakingRange)
		return;

	auto* PlayerController = Cast<AMPlayerController>(OwnerCharacter->GetController()); check(PlayerController);
	auto* InventoryController = PlayerController->GetInventoryControllerComponent(); check(InventoryController);

	InventoryController->CloseCommunicationWidget();

	IN_InterlocutorCharacter->GetCommunicationComponent()->SetInterlocutorCharacter(OwnerCharacter);
	SetInterlocutorCharacter(IN_InterlocutorCharacter);

	GenerateInventoryToReward(OwnerCharacter->GetInventoryToOfferComponent(), OwnerCharacter->GetInventoryToRewardComponent());
	// TODO: Remove delegate bindings on StopSpeaking
	OwnerCharacter->GetInventoryToOfferComponent()->OnAnySlotChangedDelegate.AddLambda([this, OwnerCharacter, PlayerController]
	{
		GenerateInventoryToReward(OwnerCharacter->GetInventoryToOfferComponent(), OwnerCharacter->GetInventoryToRewardComponent());
		if (PlayerController)
		{
			if (auto* InventoryController = PlayerController->GetInventoryControllerComponent())
			{
				InventoryController->UpdateCommunicationWidget();
			}
		}
	});

	InventoryController->CreateCommunicationWidget();
}

void UMCommunicationComponent::StopSpeaking()
{
	const auto OwnerCharacter = Cast<AMCharacter>(GetOwner());
	if (!IsValid(OwnerCharacter))
		return;

	// It may also be a non-player AMCharacter. TODO: Handle this when refactoring for generic communication widget
	if (auto* PlayerController = Cast<AMPlayerController>(OwnerCharacter->GetController()))
	{
		if (auto* InventoryController = PlayerController->GetInventoryControllerComponent())
		{
			InventoryController->CloseCommunicationWidget();

			// TODO: Refactor using only CommunicationComponent
			if (InterlocutorCharacter)
			{
				InterlocutorCharacter->GetCommunicationComponent()->SetInterlocutorCharacter(nullptr);
				SetInterlocutorCharacter(nullptr);
			}

			CancelOffer(OwnerCharacter->GetInventoryToOfferComponent());
		}
	}
}

void UMCommunicationComponent::GenerateInventoryToReward(const UMInventoryComponent* InventoryToOffer, UMInventoryComponent* InventoryToReward) const
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

void UMCommunicationComponent::MakeADeal(UMInventoryComponent* InventoryToOffer,
	UMInventoryComponent* InventoryToReward)
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

void UMCommunicationComponent::CancelOffer(UMInventoryComponent* InventoryToOffer) const
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

void UMCommunicationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Check if the owner had had a conversation with some mob but ran away
	if (InterlocutorCharacter)
	{
		if (const auto OwnerCharacter = Cast<AMCharacter>(GetOwner()))
		{
			const float SpeakingRange = OwnerCharacter->GetStatsModelComponent()->GetSpeakingRange();
			if (FVector::Dist(OwnerCharacter->GetActorLocation(), InterlocutorCharacter->GetActorLocation()) > SpeakingRange)
			{
				StopSpeaking();
			}
		}
	}
}
