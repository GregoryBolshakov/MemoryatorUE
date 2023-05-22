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
#include "Components/Button.h"

void UMCommunicationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (const auto pWorld = GetWorld())
	{
		if (const auto WorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto WorldGenerator = WorldManager->GetWorldGenerator())
			{
				if (const auto CommunicationManager = WorldGenerator->GetCommunicationManager(); CommunicationManager)
				{
					pTakeAllButton->OnClicked.AddDynamic(CommunicationManager, &AMCommunicationManager::MakeADeal);
				}
			}
		}
	}
}

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
					pTakeAllButton->OnClicked.RemoveAll(CommunicationManager);
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
			}
		}
	}
	check(false);
}
