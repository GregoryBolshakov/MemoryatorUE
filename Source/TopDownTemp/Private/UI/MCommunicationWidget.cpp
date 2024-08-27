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
		// TODO: Finish refactoring. MInventoryControllerComponent should call MakeADeal, maybe
		//pTakeAllButton->OnClicked.AddDynamic(CommunicationManager, &AMCommunicationManager::MakeADeal);
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

void UMCommunicationWidget::CreateSlots(const UMInventoryComponent* InventoryToOffer, const UMInventoryComponent* InventoryToReward)
{
	if (!ItemSlotWidgetBPClass || !pMyItemSlotsWrapBox || !pTheirItemSlotsWrapBox || !pRewardItemSlotsWrapBox || !InventoryToOffer || !InventoryToReward)
	{
		check(false);
		return;
	}

	const auto* pWorld = GetWorld();
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

	UMInventoryWidget::CreateItemSlotWidgets(this, InventoryToOffer, pMyItemSlotsWrapBox);

	const auto* InterlocutorInventory = InterlocutorCharacter->GetInventoryComponent();
	if (!InterlocutorInventory) return;

	UMInventoryWidget::CreateItemSlotWidgets(this, InterlocutorInventory, pTheirItemSlotsWrapBox);

	UMInventoryWidget::CreateItemSlotWidgets(this, InventoryToReward, pRewardItemSlotsWrapBox);
}
