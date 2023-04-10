// Copyright Epic Games, Inc. All Rights Reserved.

#include "MInventorySlotWidget.h"
#include "MDropManager.h"
#include "MWorldManager.h"
#include "MWorldGenerator.h"
#include "MInventoryWidget.h"

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
