#include "MCommunicationWidget.h"
#include "MCharacter.h"
#include "MInventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/WrapBox.h"
#include "MInventoryComponent.h"
#include "MCommunicationManager.h"
#include "MInventoryWidget.h"
#include "MWorldGenerator.h"
#include "MWorldManager.h"

void UMCommunicationWidget::NativeDestruct()
{
	Super::NativeDestruct();

	if (const auto pPlayerCharacter = Cast<AMCharacter>(GetOwningPlayerPawn()))
	{
		if (const auto InventoryComponent = pPlayerCharacter->GetInventoryComponent())
		{
			for (auto& ItemSlot : InventoryComponent->GetSlots())
			{
				ItemSlot.OnSlotChangedDelegate.Unbind();
			}
		}
	}

	if (const auto pWorld = GetWorld())
	{
		if (const auto WorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto WorldGenerator = WorldManager->GetWorldGenerator())
			{
				if (const auto CommunicationManager = WorldGenerator->GetCommunicationManager(); CommunicationManager)
				{
					CommunicationManager->StopSpeaking();
				}
			}
		}
	}
}

void UMCommunicationWidget::CreateItemSlotWidgets()
{
	if (!ItemSlotWidgetBPClass || !pMyItemSlotsWrapBox || !pTheirItemSlotsWrapBox || !pRewardItemSlotsWrapBox)
	{
		check(false);
		return;
	}

	const auto pWorld = GetWorld();
	if (!pWorld) { check(false); return; }

	const AMCommunicationManager* CommunicationManager = nullptr;
	const AMCharacter* InterlocutorCharacter = nullptr;
	if (const auto WorldManager = pWorld->GetSubsystem<UMWorldManager>())
	{
		if (const auto WorldGenerator = WorldManager->GetWorldGenerator())
		{
			if (CommunicationManager = WorldGenerator->GetCommunicationManager(); CommunicationManager)
			{
				if (InterlocutorCharacter = CommunicationManager->GetInterlocutorCharacter(); !InterlocutorCharacter)
				{
					return;
				}
			}
		}
	}

	const auto InventoryToOffer = CommunicationManager->GetInventoryToOffer(); // Place player can put their items to offer
	if (!InventoryToOffer) return;

	UMInventoryWidget::CreateItemSlotWidgets(this, InventoryToOffer, pMyItemSlotsWrapBox);

	const auto InterlocutorInventory = InterlocutorCharacter->GetInventoryComponent();
	if (!InterlocutorInventory) return;

	UMInventoryWidget::CreateItemSlotWidgets(this, InterlocutorInventory, pTheirItemSlotsWrapBox);

	const auto InventoryToReward = CommunicationManager->GetInventoryToReward();
	if (!InventoryToReward) return;

	UMInventoryWidget::CreateItemSlotWidgets(this, InventoryToReward, pRewardItemSlotsWrapBox);
}

void UMCommunicationWidget::ReCreateRewardItemSlotWidgets()
{
	if (const auto pWorld = GetWorld())
	{
		if (const auto WorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto WorldGenerator = WorldManager->GetWorldGenerator())
			{
				if (const auto CommunicationManager = WorldGenerator->GetCommunicationManager())
				{
					if (const auto InventoryToReward = CommunicationManager->GetInventoryToReward())
					{
						pRewardItemSlotsWrapBox->ClearChildren();
						UMInventoryWidget::CreateItemSlotWidgets(this, InventoryToReward, pRewardItemSlotsWrapBox);
						return;
					}
				}
			}
		}
	}
	check(false);
}
