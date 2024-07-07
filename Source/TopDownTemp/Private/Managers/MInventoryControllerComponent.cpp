#include "MInventoryControllerComponent.h"

#include "MDropManager.h"
#include "Characters/MCharacter.h"
#include "Controllers/MPlayerController.h"
#include "Framework/MGameMode.h"
#include "Net/UnrealNetwork.h"
#include "UI/MInventoryWidget.h"
#include "UI/MPickUpBarWidget.h"

void UMInventoryControllerComponent::Server_TryDropDraggedOnTheGround_Implementation()
{
	if (auto* MyInventory = GetMyInventory())
	{
		MyInventory->DropDraggedOnTheGround(DraggedItem);
	}
}

void UMInventoryControllerComponent::Server_TryStoreDraggedToAnySlot_Implementation(FMUid InventoryOwnerActorUid)
{
	if (!IsUidValid(InventoryOwnerActorUid)) // Drop dragged on the ground if the owner isn't set
	{
		if (auto* MyInventory = GetMyInventory())
		{
			MyInventory->DropDraggedOnTheGround(DraggedItem);
			return;
		}
	}
	if (auto* InventoryOwnerMetadata = AMGameMode::GetMetadataManager(this)->Find(InventoryOwnerActorUid))
	{
		if (auto* Inventory = InventoryOwnerMetadata->Actor->GetComponentByClass<UMInventoryComponent>())
		{
			Inventory->StoreDraggedToAnySlot(DraggedItem);
			return;
		}
	}
	check(false);
}

void UMInventoryControllerComponent::Server_TryStoreDraggedToSpecificSlot_Implementation(FMUid InventoryOwnerActorUid, int SlotNumberInArray)
{
	if (DraggedItem.Quantity == 0) // An example of that is right after Server_TrySwapDraggedWithSpecificSlot
		return;

	if (!IsUidValid(InventoryOwnerActorUid)) // Drop dragged on the ground if the owner isn't set
	{
		if (auto* MyInventory = GetMyInventory())
		{
			MyInventory->DropDraggedOnTheGround(DraggedItem);
			return;
		}
	}
	if (auto* InventoryOwnerMetadata = AMGameMode::GetMetadataManager(this)->Find(InventoryOwnerActorUid))
	{
		if (auto* Inventory = InventoryOwnerMetadata->Actor->GetComponentByClass<UMInventoryComponent>())
		{
			Inventory->StoreDraggedToSpecificSlot(SlotNumberInArray, DraggedItem);
			return;
		}
	}
	check(false);
}

void UMInventoryControllerComponent::Server_TryDragItemFromSpecificSlot_Implementation(FMUid InventoryOwnerActorUid, int SlotNumberInArray, int Quantity)
{
	check(DraggedItem.Quantity == 0);
	if (auto* InventoryOwnerMetadata = AMGameMode::GetMetadataManager(this)->Find(InventoryOwnerActorUid))
	{
		if (auto* Inventory = InventoryOwnerMetadata->Actor->GetComponentByClass<UMInventoryComponent>())
		{
			DraggedItem = Inventory->DragItemFromSpecificSlot(SlotNumberInArray, Quantity);
			return;
		}
	}
	check(false);
}

void UMInventoryControllerComponent::Server_TrySwapDraggedWithSpecificSlot_Implementation(FMUid InventoryOwnerActorUid, int SlotNumberInArray)
{
	check(DraggedItem.Quantity != 0);
	if (auto* InventoryOwnerMetadata = AMGameMode::GetMetadataManager(this)->Find(InventoryOwnerActorUid))
	{
		if (auto* Inventory = InventoryOwnerMetadata->Actor->GetComponentByClass<UMInventoryComponent>())
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

UMInventoryComponent* UMInventoryControllerComponent::GetMyInventory() const
{
	if (const auto* Controller = Cast<AController>(GetOwner()))
	{
		if (const auto* MCharacter = Cast<AMCharacter>(Controller->GetPawn()))
		{
			return MCharacter->GetInventoryComponent();
		}
	}
	check(false);
	return nullptr;
}
