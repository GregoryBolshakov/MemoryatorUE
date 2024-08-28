#include "MCommunicationWidget.h"
#include "Characters/MCharacter.h"
#include "UI/MInventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/WrapBox.h"
#include "Components/MInventoryComponent.h"
#include "Managers/MCommunicationManager.h"
#include "UI/MInventoryWidget.h"
#include "Managers/MWorldGenerator.h"
#include "Components/Button.h"
#include "Components/MCommunicationComponent.h"
#include "Framework/MGameMode.h"

// So far we don't support cross mob communication, and the widget belongs to Player.
// But it is very likely we will start supporting it.

void UMCommunicationWidget::NativeDestruct()
{
	Super::NativeDestruct();

	// TODO: Unbind delegate bindings if any
}

void UMCommunicationWidget::CreateSlots(const UMInventoryComponent* InventoryToOffer, const UMInventoryComponent* InventoryToReward)
{
	if (!ItemSlotWidgetBPClass || !pMyItemSlotsWrapBox || !pTheirItemSlotsWrapBox || !pRewardItemSlotsWrapBox || !InventoryToOffer || !InventoryToReward)
	{
		check(false);
		return;
	}

	if (auto* PlayerCharacter = Cast<AMCharacter>(GetOwningPlayerPawn()))
	{
		if (auto* CommunicationComponent = PlayerCharacter->GetCommunicationComponent())
		{
			if (const auto* InterlocutorCharacter = CommunicationComponent->GetInterlocutorCharacter())
			{
				UMInventoryWidget::CreateItemSlotWidgets(this, InventoryToOffer, pMyItemSlotsWrapBox);

				const auto* InterlocutorInventory = InterlocutorCharacter->GetInventoryComponent();
				if (!InterlocutorInventory) return;

				UMInventoryWidget::CreateItemSlotWidgets(this, InterlocutorInventory, pTheirItemSlotsWrapBox);

				UMInventoryWidget::CreateItemSlotWidgets(this, InventoryToReward, pRewardItemSlotsWrapBox);
			}
		}
	}
}
