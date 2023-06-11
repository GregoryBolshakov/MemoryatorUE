// Copyright Epic Games, Inc. All Rights Reserved.

#include "MInventorySlotWidget.h"
#include "MInventoryWidget.h"
#include "Managers/MDropManager.h"
#include "Managers/MWorldManager.h"
#include "Managers/MWorldGenerator.h"

void UMInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				if (pDropManager = pWorldGenerator->GetDropManager(); !pDropManager)
				{
					check(false);
				}
			}
		}
	}
}
