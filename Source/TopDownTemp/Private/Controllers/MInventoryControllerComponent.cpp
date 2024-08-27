#include "MInventoryControllerComponent.h"

#include "Managers/MDropManager.h"
#include "Characters/MCharacter.h"
#include "Controllers/MPlayerController.h"
#include "Framework/MGameMode.h"
#include "Net/UnrealNetwork.h"
#include "UI/MCommunicationWidget.h"
#include "UI/MInventoryWidget.h"
#include "UI/MPickUpBarWidget.h"

UMInventoryComponent* GetRequestedInventory(AActor* Actor, EInventoryType InventoryType)
{
	if (auto* MCharacter = Cast<AMCharacter>(Actor))
	{
		// MCharacters have multiple types of inventories (the main one, for trading, for taking gifts, etc.)
		auto* Inventory = MCharacter->GetInventoryByType(InventoryType);
		check(Inventory);
		return Inventory;
	}

	// MActors have only main inventory
	check(InventoryType == EInventoryType::Main);
	auto* Inventory = Actor->GetComponentByClass<UMInventoryComponent>();
	check(Inventory);
	return Inventory;
}

void UMInventoryControllerComponent::Server_TryDropDraggedOnTheGround_Implementation(const EInventoryType InventoryType)
{
	if (auto* MyInventory = GetMyInventory(InventoryType))
	{
		MyInventory->DropDraggedOnTheGround(DraggedItem);
	}
}

void UMInventoryControllerComponent::Server_TryStoreDraggedToAnySlot_Implementation(FMUid InventoryOwnerActorUid, const EInventoryType InventoryType)
{
	if (!IsUidValid(InventoryOwnerActorUid)) // Drop dragged on the ground if the owner isn't set
	{
		if (auto* MyInventory = GetMyInventory(InventoryType))
		{
			MyInventory->DropDraggedOnTheGround(DraggedItem);
			return;
		}
	}
	if (auto* InventoryOwnerMetadata = AMGameMode::GetMetadataManager(this)->Find(InventoryOwnerActorUid))
	{
		if (auto* Inventory = GetRequestedInventory(InventoryOwnerMetadata->Actor, InventoryType))
		{
			Inventory->StoreDraggedToAnySlot(DraggedItem);
			return;
		}
	}
	check(false);
}

void UMInventoryControllerComponent::Server_TryStoreDraggedToSpecificSlot_Implementation(FMUid InventoryOwnerActorUid, int SlotNumberInArray, const EInventoryType InventoryType)
{
	if (DraggedItem.Quantity == 0) // An example of that is right after Server_TrySwapDraggedWithSpecificSlot
		return;

	if (!IsUidValid(InventoryOwnerActorUid)) // Drop dragged on the ground if the owner isn't set
	{
		if (auto* MyInventory = GetMyInventory(InventoryType))
		{
			MyInventory->DropDraggedOnTheGround(DraggedItem);
			return;
		}
	}
	if (auto* InventoryOwnerMetadata = AMGameMode::GetMetadataManager(this)->Find(InventoryOwnerActorUid))
	{
		if (auto* Inventory = GetRequestedInventory(InventoryOwnerMetadata->Actor, InventoryType))
		{
			Inventory->StoreDraggedToSpecificSlot(SlotNumberInArray, DraggedItem);
			return;
		}
	}
	check(false);
}

void UMInventoryControllerComponent::Server_TryDragItemFromSpecificSlot_Implementation(FMUid InventoryOwnerActorUid, int SlotNumberInArray, int Quantity, const EInventoryType InventoryType)
{
	check(DraggedItem.Quantity == 0);
	if (auto* InventoryOwnerMetadata = AMGameMode::GetMetadataManager(this)->Find(InventoryOwnerActorUid))
	{
		if (auto* Inventory = GetRequestedInventory(InventoryOwnerMetadata->Actor, InventoryType))
		{
			DraggedItem = Inventory->DragItemFromSpecificSlot(SlotNumberInArray, Quantity);
			return;
		}
	}
	check(false);
}

void UMInventoryControllerComponent::Server_TrySwapDraggedWithSpecificSlot_Implementation(FMUid InventoryOwnerActorUid, int SlotNumberInArray, const EInventoryType InventoryType)
{
	check(DraggedItem.Quantity != 0);
	if (auto* InventoryOwnerMetadata = AMGameMode::GetMetadataManager(this)->Find(InventoryOwnerActorUid))
	{
		if (auto* Inventory = GetRequestedInventory(InventoryOwnerMetadata->Actor, InventoryType))
		{
			Inventory->SwapItems(DraggedItem, SlotNumberInArray);
			return;
		}
	}
	check(false);
}

void UMInventoryControllerComponent::AddInventoryForPickUp(const UMInventoryComponent* ReplicatedInventory)
{
	if (ReplicatedInventory->GetSlotsConst().IsEmpty() || !GetWorld() || !GetWorld()->GetFirstPlayerController())
		return;

	if (InventoriesToRepresent.IsEmpty())
	{
		if (PickUpBarWidget)
		{
			PickUpBarWidget->Show();
		}
		else
		{
			//PlayerController->Client_ReceiveHUDCommand(FHUDCommand(EHUDCommandType::CreateWidget, "PickUpBar"));
			PickUpBarWidget = Cast<UMPickUpBarWidget>(CreateWidget(GetWorld()->GetFirstPlayerController(), UMDropManager::gPickUpBarWidgetBPClass));
			check(PickUpBarWidget);
			PickUpBarWidget->AddToPlayerScreen();
		}
		
	}

	InventoriesToRepresent.Add(ReplicatedInventory);

	PickUpBarWidget->CreateSlots(InventoriesToRepresent);
}

void UMInventoryControllerComponent::RemoveInventoryForPickUp(const UMInventoryComponent* ReplicatedInventory)
{
	InventoriesToRepresent.Remove(ReplicatedInventory);

	if (!PickUpBarWidget)
	{
		check(false);
		return;
	}

	if (!InventoriesToRepresent.IsEmpty())
	{
		PickUpBarWidget->CreateSlots(InventoriesToRepresent);
	}
	else
	{
		PickUpBarWidget->Hide();
	}
}

void UMInventoryControllerComponent::UpdatePickUpBar() const
{
	if (!InventoriesToRepresent.IsEmpty() && PickUpBarWidget)
	{
		PickUpBarWidget->CreateSlots(InventoriesToRepresent);
	}
}

void UMInventoryControllerComponent::UpdateInventoryWidget() const
{
	if (const auto* PlayerController = Cast<APlayerController>(GetOwner()))
	{
		if (const auto* MCharacter = Cast<AMCharacter>(PlayerController->GetPawn()))
		{
			if (const auto InventoryComponent = MCharacter->GetInventoryComponent())
			{
				if (InventoryWidget)
				{
					InventoryWidget->CreateSlots(InventoryComponent);
				}
			}
		}
	}
}

void UMInventoryControllerComponent::UpdateCommunicationWidget() const
{
	if (const auto* PlayerController = Cast<APlayerController>(GetOwner()))
	{
		if (const auto* MCharacter = Cast<AMCharacter>(PlayerController->GetPawn()))
		{
			const auto InventoryToOfferComponent = MCharacter->GetInventoryToOfferComponent();
			const auto InventoryToRewardComponent = MCharacter->GetInventoryToRewardComponent();
			if (InventoryToOfferComponent && InventoryToRewardComponent)
			{
				if (CommunicationWidget)
				{
					CommunicationWidget->CreateSlots(InventoryToOfferComponent, InventoryToRewardComponent);
				}
			}
		}
	}
}

void UMInventoryControllerComponent::CreateOrShowInventoryWidget()
{
	if (!InventoryWidget)
	{
		auto* Controller = Cast<APlayerController>(GetOwner());
		InventoryWidget = CreateWidget<UMInventoryWidget>(Controller, UMDropManager::gInventoryWidgetBPClass, TEXT("InventoryWidget"));
		InventoryWidget->AddToPlayerScreen();
		UpdateInventoryWidget();
	}
	else
	{
		InventoryWidget->Show();
	}
}

void UMInventoryControllerComponent::CreateCommunicationWidget()
{
	auto* Controller = Cast<APlayerController>(GetOwner());
	CommunicationWidget = CreateWidget<UMCommunicationWidget>(Controller, UMDropManager::gCommunicationWidgetBPClass, TEXT("CommunicationWidget"));
	CommunicationWidget->AddToPlayerScreen();
	UpdateCommunicationWidget();
}

void UMInventoryControllerComponent::CloseCommunicationWidget() const
{
	if (CommunicationWidget)
	{
		CommunicationWidget->Close();
	}
}

UMInventoryComponent* UMInventoryControllerComponent::GetMyInventory(EInventoryType InventoryType) const
{
	if (const auto* Controller = Cast<AController>(GetOwner()))
	{
		return GetRequestedInventory(Controller->GetPawn(), InventoryType);
	}
	check(false);
	return nullptr;
}
