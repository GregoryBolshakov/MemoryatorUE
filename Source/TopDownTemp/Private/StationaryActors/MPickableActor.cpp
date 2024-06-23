#include "MPickableActor.h"

#include "Components/M2DRepresentationComponent.h"
#include "Managers/MDropManager.h"
#include "PaperSprite.h"
#include "Characters/MMemoryator.h"
#include "Managers/MWorldGenerator.h"
#include "Components/CapsuleComponent.h"
#include "Framework/MGameMode.h"
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
}

void AMPickableActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (InventoryComponent->GetItemCopies().IsEmpty())
		return;

	if (const auto pWorld = GetWorld())
	{
		if (const auto pPlayerPawn = UGameplayStatics::GetPlayerPawn(pWorld, 0);
			pPlayerPawn && pPlayerPawn == OtherActor)
		{
			AMGameMode::GetDropManager(this)->AddInventory(InventoryComponent);
		}
	}
}

void AMPickableActor::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (const auto pWorld = GetWorld())
	{
		if (const auto pPlayerPawn = UGameplayStatics::GetPlayerPawn(pWorld, 0);
			pPlayerPawn && pPlayerPawn == OtherActor)
		{
			AMGameMode::GetDropManager(this)->RemoveInventory(InventoryComponent);
		}
	}
}

FTimerHandle CollisionTimerHandle;
void AMPickableActor::BeginPlay()
{
	Super::BeginPlay();

	// NotifyActorBeginOverlap doesn't trigger when actor just spawned
	/*GetWorld()->GetTimerManager().SetTimer(CollisionTimerHandle, [this]
	{
		if (const auto pWorld = GetWorld())
		{
			if (const auto pPlayerPawn = UGameplayStatics::GetPlayerPawn(pWorld, 0))
			{
				if (IsOverlappingActor(pPlayerPawn))
				{
					NotifyActorBeginOverlap(pPlayerPawn);
				}
			}
		}
	}, 0.1f, false); // Who knows why collisions need some time after actor's BeginPlay to be set up*/
}

void AMPickableActor::OnItemChanged(int NewItemID, int NewQuantity)
{
	if (InventoryComponent->GetItemCopies().IsEmpty() && bDisappearIfEmptyInventory)
	{
		PickedUpCompletelyDelegate.Broadcast(GetClass());
		AMGameMode::GetDropManager(this)->RemoveInventory(InventoryComponent);
		Destroy();
		return;
	}

	AMGameMode::GetDropManager(this)->Update();
}
