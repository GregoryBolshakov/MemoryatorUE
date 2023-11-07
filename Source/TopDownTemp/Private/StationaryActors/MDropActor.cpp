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

void AMDropActor::InitialiseInventory(const TArray<FItem>& IN_Items)
{
	Super::InitialiseInventory(IN_Items);

	if (IN_Items.Num() != 1 || IN_Items[0].ID <= 0 || IN_Items[0].Quantity <= 0)
	{
		check(false);
		return;
	}

	// Try to set sprite and quantity text using the data asset stored in the UMGameInstance
	if (const auto TaggedComponents = GetComponentsByTag(UPaperSpriteComponent::StaticClass(), "ItemSprite");
		!TaggedComponents.IsEmpty())
	{
		if (auto SpriteComponent = Cast<UPaperSpriteComponent>(TaggedComponents[0]))
		{
			if (const auto pGameInstace = GetGameInstance<UMGameInstance>();
				pGameInstace->ItemsDataAsset && pGameInstace->ItemsDataAsset->ItemsData.Num() > IN_Items[0].ID)
			{
				const auto ItemData = pGameInstace->ItemsDataAsset->ItemsData[IN_Items[0].ID];

				SpriteComponent->SetSprite(ItemData.IconSprite);
			}
		}
	}
	else
	{
		check(false);
	}
}
