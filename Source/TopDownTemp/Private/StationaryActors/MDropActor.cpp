// Copyright Epic Games, Inc. All Rights Reserved.

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

	//TODO: Looks like hack. Get rid of this
	if (const auto pWorld = GetWorld(); pDropManager)
	{
		if (const auto pPlayerPawn = UGameplayStatics::GetPlayerPawn(pWorld, 0))
		{
			if (const auto Capsule = Cast<UCapsuleComponent>(GetDefaultSubobjectByName(TEXT("CollectScopeCapsule"))))
			{
				auto Location2D = GetActorLocation();
				Location2D.Z = 0.f;
				auto PlayerLocation2D = pPlayerPawn->GetActorLocation();
				PlayerLocation2D.Z = 0.f;
				if (FVector::Distance(Location2D, PlayerLocation2D) < Capsule->GetScaledCapsuleRadius())
				{
					NotifyActorBeginOverlap(pPlayerPawn);
				}
			}
		}
	}
}
