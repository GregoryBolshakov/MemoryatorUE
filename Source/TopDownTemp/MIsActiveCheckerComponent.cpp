#include "MIsActiveCheckerComponent.h"
#include "MWorldManager.h"

UMIsActiveCheckerComponent::UMIsActiveCheckerComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	bIsActive(false),
	Scope(nullptr)
{
}

void UMIsActiveCheckerComponent::Disable()
{
	auto pOwner = GetOwner();
	check(pOwner);

	pOwner->SetActorHiddenInGame(true);
	pOwner->SetActorTickEnabled(false);

	bIsActive = false;
}

void UMIsActiveCheckerComponent::Enable()
{
	auto pOwner = GetOwner();
	check(pOwner);

	pOwner->SetActorHiddenInGame(false);
	pOwner->SetActorTickEnabled(true);

	bIsActive = true;
}

void UMIsActiveCheckerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	auto MarkedComponents = GetOwner()->GetComponentsByTag(UShapeComponent::StaticClass(), "IsActiveChecker");
	check(!MarkedComponents.IsEmpty());

	Scope = Cast<UShapeComponent>(MarkedComponents[0]);
	check(Scope);

	Scope->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Scope->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Scope->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	Scope->SetGenerateOverlapEvents(true);
}
