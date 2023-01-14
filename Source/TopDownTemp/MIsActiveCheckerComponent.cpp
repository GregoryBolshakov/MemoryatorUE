#include "MIsActiveCheckerComponent.h"

#include "Components/PrimitiveComponent.h"

UMIsActiveCheckerComponent::UMIsActiveCheckerComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	bIsActive(false),
	CollisionPrimitive(nullptr)
{
}

void UMIsActiveCheckerComponent::Disable()
{
	auto pOwner = GetOwner();
	if (!pOwner)
	{
		check(false);
		return;
	}

	pOwner->SetActorHiddenInGame(true);
	pOwner->SetActorTickEnabled(false);

	bIsActive = false;
}

void UMIsActiveCheckerComponent::Enable()
{
	auto pOwner = GetOwner();
	if (!pOwner)
	{
		check(false);
		return;
	}

	pOwner->SetActorHiddenInGame(false);
	pOwner->SetActorTickEnabled(true);

	bIsActive = true;
}

void UMIsActiveCheckerComponent::SetUpCollisionPrimitive()
{
	auto MarkedComponents = GetOwner()->GetComponentsByTag(UPrimitiveComponent::StaticClass(), "IsActiveChecker");
	if (MarkedComponents.IsEmpty())
	{
		check(false);
		return;
	}

	CollisionPrimitive = Cast<UPrimitiveComponent>(MarkedComponents[0]);
	if (!CollisionPrimitive)
	{
		check(false);
		return;
	}

	CollisionPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionPrimitive->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	CollisionPrimitive->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	CollisionPrimitive->SetGenerateOverlapEvents(true);
}
