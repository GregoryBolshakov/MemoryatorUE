#include "MDropManager.h"
#include "UI/MPickUpBarWidget.h"
#include "StationaryActors/MPickableActor.h"
#include "MWorldGenerator.h"
#include "Characters/MCharacter.h"
#include "Components/MInventoryComponent.h"
#include "Controllers/MHUDController.h"
#include "Controllers/MPlayerController.h"
#include "Framework/MGameInstance.h"
#include "Framework/MGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "NakamaManager/Private/ShopManagerClient.h"

UMDropManager::UMDropManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (const auto World = UObject::GetWorld())
	{
		if (const auto GameInstance = World->GetGameInstance<UMGameInstance>())
		{
			if (const auto NakamaManager = GameInstance->GetNakamaManager())
			{
				NakamaManager->ShopManagerClient->OnBundleTxnFinalizedDelegate.BindUObject(this, &UMDropManager::GiveBundleToPlayer);
			}
		}
	}
}

void UMDropManager::AddInventory(UMInventoryComponent* Inventory, AMPlayerController* PlayerController)
{
	if (Inventory->GetSlots().IsEmpty() || !GetWorld() || !GetWorld()->GetFirstPlayerController())
		return;

	if (InventoriesToRepresent.IsEmpty())
	{
		if (PickUpBarWidget)
		{
			PickUpBarWidget->Show();
		}
		else
		{
			//PlayerController->Client_ReceiveHUDCommand(FHUDCommand(EHUDCommandType::CreateWidget, "PickUpBar"));
			PickUpBarWidget = Cast<UMPickUpBarWidget>(CreateWidget(GetWorld()->GetFirstPlayerController(), PickUpBarWidgetBPClass));
			check(PickUpBarWidget);
			PickUpBarWidget->AddToPlayerScreen();
		}
		
	}

	InventoriesToRepresent.Add(Inventory);

	PickUpBarWidget->CreateSlots(InventoriesToRepresent);
}

void UMDropManager::RemoveInventory(UMInventoryComponent* Inventory, AMPlayerController* PlayerController)
{
	InventoriesToRepresent.Remove(Inventory);

	if (!PickUpBarWidget)
	{
		check(false);
		return;
	}

	if (!InventoriesToRepresent.IsEmpty())
	{
		PickUpBarWidget->CreateSlots(InventoriesToRepresent);
	}
	else
	{
		PickUpBarWidget->Hide();
	}
}

void UMDropManager::Update()
{
	if (!InventoriesToRepresent.IsEmpty() && PickUpBarWidget)
	{
		PickUpBarWidget->CreateSlots(InventoriesToRepresent);
	}
}

void UMDropManager::SpawnPickableItem(const FItem& Item)
{
	check(Item.Quantity != 0);
	const auto pWorld = GetWorld();
	if (!IsValid(pWorld))
		return;

	if (const auto WorldGenerator = AMGameMode::GetWorldGenerator(this))
	{
		const auto pPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
		if (!pPlayer) { check(false); return; }
		const auto PlayerLocation = pPlayer->GetActorLocation();

		FOnActorSpawned OnActorSpawned;
		OnActorSpawned.AddLambda([Item](AActor* Actor)
		{
			if (const auto PickableActor = Cast<AMPickableActor>(Actor))
			{
				PickableActor->InitialiseInventory({Item});
			}
		});
		WorldGenerator->SpawnActorInRadius<AMPickableActor>(AMPickableItemBPClass, PlayerLocation, FRotator::ZeroRotator, {}, 25.f, 0.f, OnActorSpawned);
	}
}

void UMDropManager::GiveBundleToPlayer(const FBundle& Bundle)
{
	if (const auto pPlayer = Cast<AMCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		if (const auto InventoryComponent = pPlayer->GetInventoryComponent())
		{
			for (const auto& Item : Bundle.items)
			{
				InventoryComponent->StoreItem({int(Item.itemid), Item.qty});
			}
		}
	}
}

TSubclassOf<UUserWidget> UMDropManager::gItemSlotWidgetBPClass = nullptr;

void UMDropManager::PostInitProperties()
{
	UObject::PostInitProperties();

	UMDropManager::gItemSlotWidgetBPClass = ItemSlotWidgetBPClass;
}
