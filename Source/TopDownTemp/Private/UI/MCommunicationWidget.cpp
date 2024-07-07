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
#include "Framework/MGameMode.h"

void UMCommunicationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (const auto CommunicationManager = AMGameMode::GetCommunicationManager(this))
	{
		pTakeAllButton->OnClicked.AddDynamic(CommunicationManager, &AMCommunicationManager::MakeADeal);
	}
}

void UMCommunicationWidget::NativeDestruct()
{
	Super::NativeDestruct();

	if (const auto CommunicationManager = AMGameMode::GetCommunicationManager(this))
	{
			CommunicationManager->StopSpeaking();
			pTakeAllButton->OnClicked.RemoveAll(CommunicationManager);
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

	const AMCommunicationManager* CommunicationManager = AMGameMode::GetCommunicationManager(this);
	const AMCharacter* InterlocutorCharacter = nullptr;
	if (CommunicationManager)
	{
		if (InterlocutorCharacter = CommunicationManager->GetInterlocutorCharacter(); !InterlocutorCharacter)
		{
			return;
		}
	}

	const auto InventoryToOffer = CommunicationManager->GetInventoryToOffer(); // Place player can put their items to offer
	if (!InventoryToOffer) return;

	// TODO: Support it. Temporary disabled due to multiplayer refactoring
	//UMInventoryWidget::CreateItemSlotWidgets(this, InventoryToOffer, pMyItemSlotsWrapBox);

	const auto InterlocutorInventory = InterlocutorCharacter->GetInventoryComponent();
	if (!InterlocutorInventory) return;

	//UMInventoryWidget::CreateItemSlotWidgets(this, InterlocutorInventory, pTheirItemSlotsWrapBox);

	const auto InventoryToReward = CommunicationManager->GetInventoryToReward();
	if (!InventoryToReward) return;

	//UMInventoryWidget::CreateItemSlotWidgets(this, InventoryToReward, pRewardItemSlotsWrapBox);
}

void UMCommunicationWidget::ReCreateRewardItemSlotWidgets()
{
	if (const auto CommunicationManager = AMGameMode::GetCommunicationManager(this))
	{
		if (const auto InventoryToReward = CommunicationManager->GetInventoryToReward())
		{
			pRewardItemSlotsWrapBox->ClearChildren();
			// TODO: Support it. Temporary disabled due to multiplayer refactoring
			//UMInventoryWidget::CreateItemSlotWidgets(this, InventoryToReward, pRewardItemSlotsWrapBox);

			// Enable/Disable TakeAllButton depending on the slots locked state
			bool bHasAnyUnlocked = false;
			for (const auto ItemSlot : InventoryToReward->GetSlots())
			{
				if (!ItemSlot.CheckFlag(FSlot::ESlotFlags::Locked)) // At least one slot isn't locked
				{
					bHasAnyUnlocked = true;
					if (pTakeAllButton)
					{
						pTakeAllButton->SetVisibility(ESlateVisibility::Visible);
						pTakeAllButton->SetIsEnabled(true);
					}
					break;
				}
			}
			if (!bHasAnyUnlocked && pTakeAllButton)
			{
				pTakeAllButton->SetVisibility(ESlateVisibility::Hidden);
				pTakeAllButton->SetIsEnabled(false);
			}

			return;
		}
	}
	check(false);
}
