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
}

void AMPickableActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Try to set up the collision of the capsule determining the collectable scope
	const auto Capsule = Cast<UCapsuleComponent>(GetDefaultSubobjectByName(TEXT("CollectScopeCapsule")));
	if (Capsule)
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Capsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		Capsule->SetCollisionObjectType(ECC_Pickable);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Capsule->SetGenerateOverlapEvents(true);
	}
	check(Capsule);

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

FTimerHandle CollisionTimerHandle;
void AMPickableActor::InitialiseInventory(const TArray<FItem>& IN_Items)
{
	Super::InitialiseInventory(IN_Items);

	// NotifyActorBeginOverlap doesn't trigger when actor just spawned
	GetWorld()->GetTimerManager().SetTimer(CollisionTimerHandle, [this]
	{
		if (const auto pWorld = GetWorld(); pDropManager)
		{
			if (const auto pPlayerPawn = UGameplayStatics::GetPlayerPawn(pWorld, 0))
			{
				if (IsOverlappingActor(pPlayerPawn))
				{
					NotifyActorBeginOverlap(pPlayerPawn);
				}
			}
		}
	}, 0.1f, false); // Who knows why collisions need some time after actor's BeginPlay to be set up
}

void AMPickableActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (InventoryComponent->GetItemCopies().IsEmpty())
		return;

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

	if (InventoryComponent->GetItemCopies().IsEmpty() && bDisappearIfEmptyInventory)
	{
		PickedUpCompletelyDelegate.Broadcast(GetClass());
		pDropManager->RemoveInventory(InventoryComponent);
		Destroy();
	}

	pDropManager->Update();
}
