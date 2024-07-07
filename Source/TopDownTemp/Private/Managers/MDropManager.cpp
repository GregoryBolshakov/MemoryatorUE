#include "MDropManager.h"
#include "UI/MPickUpBarWidget.h"
#include "StationaryActors/MPickableActor.h"
#include "MWorldGenerator.h"
#include "Characters/MCharacter.h"
#include "Components/MInventoryComponent.h"
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
			PickUpBarWidget = Cast<UMPickUpBarWidget>(CreateWidget(GetWorld()->GetFirstPlayerController(), PickUpBarWidgetBPClass));
			check(PickUpBarWidget);
			PickUpBarWidget->AddToPlayerScreen();
		}
		
	}

	InventoriesToRepresent.Add(Inventory);
}

void UMDropManager::RemoveInventory(UMInventoryComponent* Inventory, AMPlayerController* PlayerController)
{
	InventoriesToRepresent.Remove(Inventory);

	if (!PickUpBarWidget)
	{
		check(false);
		return;
	}

	if (InventoriesToRepresent.IsEmpty())
	{
		PickUpBarWidget->Hide();
	}
}

void UMDropManager::SpawnPickableItem(const AActor* Owner, const FItem& Item) //TODO: Support multiple players, currently using only the first local one
{
	check(Item.Quantity != 0);
	const auto pWorld = GetWorld();
	if (!IsValid(pWorld))
		return;

	if (const auto WorldGenerator = AMGameMode::GetWorldGenerator(this))
	{
		const auto OwnerLocation = Owner->GetActorLocation();

		FOnActorSpawned OnActorSpawned;
		OnActorSpawned.AddLambda([Item](AActor* Actor)
		{
			if (const auto PickableActor = Cast<AMPickableActor>(Actor))
			{
				PickableActor->InitialiseInventory({Item});

				// Bind the single slot to OnChanged delegate
				auto& Slot = PickableActor->GetInventoryComponent()->GetSlots()[0];
				Slot.OnSlotChangedDelegate.AddDynamic(PickableActor, &AMPickableActor::OnItemChanged);
			}
		});
		WorldGenerator->SpawnActorInRadius<AMPickableActor>(AMPickableItemBPClass, OwnerLocation, FRotator::ZeroRotator, {}, 25.f, 0.f, OnActorSpawned);
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
TSubclassOf<UMPickUpBarWidget> UMDropManager::gPickUpBarWidgetBPClass = nullptr;
TSubclassOf<UMInventoryWidget> UMDropManager::gInventoryWidgetBPClass = nullptr;

void UMDropManager::PostInitProperties()
{
	UObject::PostInitProperties();

	UMDropManager::gItemSlotWidgetBPClass = ItemSlotWidgetBPClass;
	UMDropManager::gPickUpBarWidgetBPClass = PickUpBarWidgetBPClass;
	UMDropManager::gInventoryWidgetBPClass = InventoryWidgetBPClass;
}
