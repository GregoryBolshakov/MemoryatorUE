#include "MPickableActor.h"

#include "Components/M2DRepresentationComponent.h"
#include "Managers/MDropManager.h"
#include "PaperSprite.h"
#include "Characters/MMemoryator.h"
#include "Managers/MWorldGenerator.h"
#include "Components/CapsuleComponent.h"
#include "Controllers/MPlayerController.h"
#include "Framework/MGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Controllers/MInventoryControllerComponent.h"

AMPickableActor::AMPickableActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetReplicates(true);
	NetPriority = 100.f;
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
	FTimerHandle delayTimer; // This is mostly called on BeginPlay and not all data is replicated by this moment. We need a slight delay
	GetWorld()->GetTimerManager().SetTimer(delayTimer, [this, OtherActor]
	{
		Super::NotifyActorBeginOverlap(OtherActor);

		ForceNetUpdate();

		if (!HasAuthority())
			return;

		if (InventoryComponent->GetItemCopies().IsEmpty())
			return;

		if (const auto* MCharacter = Cast<AMCharacter>(OtherActor))
		{
			if (const auto MPlayerController = Cast<AMPlayerController>(MCharacter->GetController()))
			{
				MPlayerController->GetInventoryControllerComponent()->Client_AddInventoryForPickUp(InventoryComponent);
			}
		}
	}, 0.1f, false);
}

void AMPickableActor::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (!HasAuthority())
		return;

	if (const auto* MCharacter = Cast<AMCharacter>(OtherActor))
	{
		if (const auto MPlayerController = Cast<AMPlayerController>(MCharacter->GetController()))
		{
			MPlayerController->GetInventoryControllerComponent()->Client_RemoveInventoryForPickUp(Uid);
		}
	}
}

void AMPickableActor::BeginPlay()
{
	Super::BeginPlay();
}

void AMPickableActor::OnItemChanged(int NewItemID, int NewQuantity)
{
	if (InventoryComponent->GetItemCopies().IsEmpty() && bDisappearIfEmptyInventory)
	{
		PickedUpCompletelyDelegate.Broadcast(GetClass());
		Destroy();
	}
}
