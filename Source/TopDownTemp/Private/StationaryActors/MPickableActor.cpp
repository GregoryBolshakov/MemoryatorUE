#include "MPickableActor.h"

#include "Components/M2DRepresentationComponent.h"
#include "Managers/MDropManager.h"
#include "PaperSprite.h"
#include "Characters/MMemoryator.h"
#include "Managers/MWorldGenerator.h"
#include "Managers/MWorldManager.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

AMPickableActor::AMPickableActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InventoryComponent = CreateDefaultSubobject<UMInventoryComponent>(TEXT("Inventory"));
}

void AMPickableActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Try to set up the collision of the capsule determining the collectable scope 
	if (const auto Capsule = Cast<UCapsuleComponent>(GetDefaultSubobjectByName(TEXT("CollectScopeCapsule"))))
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Capsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		Capsule->SetCollisionObjectType(EEC_Pickable);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Capsule->SetGenerateOverlapEvents(true);
	}

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

void AMPickableActor::InitialiseInventory(const TArray<FItem>& IN_Items)
{
	InventoryComponent->Initialize(IN_Items.Num(), IN_Items);
}

void AMPickableActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (const auto pWorld = GetWorld(); pDropManager)
	{
		if (const auto pPlayerPawn = UGameplayStatics::GetPlayerPawn(pWorld, 0);
			pPlayerPawn && pPlayerPawn == OtherActor)
		{
			pDropManager->AddInventory(InventoryComponent);
		}
	}
}

void AMPickableActor::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (const auto pWorld = GetWorld(); pDropManager)
	{
		if (const auto pPlayerPawn = UGameplayStatics::GetPlayerPawn(pWorld, 0);
			pPlayerPawn && pPlayerPawn == OtherActor)
		{
			pDropManager->RemoveInventory(InventoryComponent);
		}
	}
}

void AMPickableActor::OnItemChanged(int NewItemID, int NewQuantity)
{

	if (!pDropManager)
	{
		check(false);
		return;
	}

	bool IsEmpty = true;
	for (const auto Slot : InventoryComponent->GetSlots())
	{
		if (Slot.Item.Quantity > 0)
		{
			IsEmpty = false;
			break;
		}
	}

	if (IsEmpty && bDisappearIfEmptyInventory)
	{
		PickedUpCompletelyDelegate.Broadcast(GetClass());
		pDropManager->RemoveInventory(InventoryComponent);
		Destroy();
	}

	pDropManager->Update();
}
