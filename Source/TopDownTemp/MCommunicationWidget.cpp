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
			for (auto& [Item, OnChangedDelegate, IsLocked, IsSecret] : InventoryComponent->GetSlots())
			{
				OnChangedDelegate.Unbind();
			}
		}
	}
}

void UMCommunicationWidget::CreateSlots()
{
	if (!ItemSlotWidgetBPClass || !pMyItemSlotsWrapBox || !pTheirItemSlotsWrapBox || !pRewardItemSlotsWrapBox)
		return;

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

	const auto MyInventoryComponent = CommunicationManager->GetInventory();
	if (!MyInventoryComponent) return;

	UMInventoryWidget::CreateSlots(this, MyInventoryComponent, pMyItemSlotsWrapBox);

	const auto InterlocutorInventoryComponent = InterlocutorCharacter->GetInventoryComponent();
	if (!InterlocutorInventoryComponent) return;

	UMInventoryWidget::CreateSlots(this, InterlocutorInventoryComponent, pTheirItemSlotsWrapBox);
}
