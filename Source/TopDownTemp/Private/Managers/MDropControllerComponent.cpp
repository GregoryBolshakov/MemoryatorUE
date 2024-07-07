#include "MDropControllerComponent.h"

#include "MDropManager.h"
#include "Characters/MCharacter.h"
#include "Controllers/MPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "UI/MInventoryWidget.h"
#include "UI/MPickUpBarWidget.h"


void UMDropControllerComponent::AddInventoryForPickUp(const UMInventoryComponent* ReplicatedInventory)
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

void UMDropControllerComponent::RemoveInventoryForPickUp(const UMInventoryComponent* ReplicatedInventory)
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

void UMDropControllerComponent::UpdatePickUpBar() const
{
	if (!InventoriesToRepresent.IsEmpty() && PickUpBarWidget)
	{
		PickUpBarWidget->CreateSlots(InventoriesToRepresent);
	}
}

void UMDropControllerComponent::UpdateInventoryWidget() const
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

void UMDropControllerComponent::CreateOrShowInventoryWidget()
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
