// Copyright Epic Games, Inc. All Rights Reserved.

#include "MPickableItem.h"

#include "M2DRepresentationComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "MGameInstance.h"
#include "Components/CapsuleComponent.h"

AMPickableItem::AMPickableItem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AMPickableItem::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	for (const auto& Capsule : RepresentationComponent->GetCapsuleComponentArray())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Capsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		Capsule->SetCollisionObjectType(EEC_Pickable);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Capsule->SetGenerateOverlapEvents(true);
	}
}

void AMPickableItem::SetItem(const FItem& IN_Item)
{
	Item = IN_Item;

	if (const auto TaggedComponents = GetComponentsByTag(UPaperSpriteComponent::StaticClass(), "ItemSprite");
		!TaggedComponents.IsEmpty())
	{
		if (auto SpriteComponent = Cast<UPaperSpriteComponent>(TaggedComponents[0]))
		{
			if (const auto pGameInstace = GetGameInstance<UMGameInstance>();
				pGameInstace->ItemsDataAsset && pGameInstace->ItemsDataAsset->ItemsData.Num() > Item.ID)
			{
				const auto ItemData = pGameInstace->ItemsDataAsset->ItemsData[Item.ID];

				SpriteComponent->SetSprite(ItemData.IconSprite);
			}
		}
	}
	else
	{
		check(false);
	}
}

void AMPickableItem::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
}
