#include "MInventorySlotWidget.h"
#include "MInventoryWidget.h"
#include "Characters/MCharacter.h"
#include "Framework/MGameMode.h"
#include "Managers/MDropManager.h"
#include "Managers/MWorldGenerator.h"
#include "StationaryActors/MActor.h"

void UMInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

FMUid UMInventorySlotWidget::GetOwnerInventoryActorUid() const
{
	check(OwnerInventory);
	if (!IsValid(OwnerInventory) || !IsValid(OwnerInventory->GetOwner()))
	{
		return {};
	}
	if (auto MActor = Cast<AMActor>(OwnerInventory->GetOwner()))
	{
		return MActor->GetUid();
	}
	if (auto MCharacter = Cast<AMCharacter>(OwnerInventory->GetOwner()))
	{
		return MCharacter->GetUid();
	}
	return {};
}
