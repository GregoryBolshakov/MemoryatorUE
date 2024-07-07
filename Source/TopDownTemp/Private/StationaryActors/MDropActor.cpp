#include "MDropActor.h"

#include "Components/M2DRepresentationComponent.h"
#include "Managers/MDropManager.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Framework/MGameInstance.h"
#include "Characters/MMemoryator.h"
#include "Managers/MWorldGenerator.h"
#include "Components/CapsuleComponent.h"
#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"

void AMDropActor::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(InventoryComponent) || ActorHasTag("DummyForDefaultBounds"))
		return;

	const auto InventorySlots = InventoryComponent->GetSlots();

	if (InventorySlots.Num() != 1 || InventorySlots[0].Item.ID <= 0 || InventorySlots[0].Item.Quantity <= 0)
	{
		if (HasAuthority())
		{
			check(false);
		} // Otherwise inventory hasn't replicated yet, it's alright
		return;
	}

	// Try to set sprite and quantity text using the data asset stored in the UMGameInstance
	if (const auto TaggedComponents = GetComponentsByTag(UPaperSpriteComponent::StaticClass(), "ItemSprite");
		!TaggedComponents.IsEmpty())
	{
		if (auto SpriteComponent = Cast<UPaperSpriteComponent>(TaggedComponents[0]))
		{
			if (const auto pGameInstace = GetGameInstance<UMGameInstance>();
				pGameInstace->ItemsDataAsset && pGameInstace->ItemsDataAsset->ItemsData.Num() > InventorySlots[0].Item.ID)
			{
				const auto ItemData = pGameInstace->ItemsDataAsset->ItemsData[InventorySlots[0].Item.ID];

				SpriteComponent->SetSprite(ItemData.IconSprite);
			}
		}
	}
	else
	{
		check(false);
	}
}
