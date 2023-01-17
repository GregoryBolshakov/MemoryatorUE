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
	const auto pOwner = GetOwner();
	if (!pOwner)
	{
		check(false);
		return;
	}

	bActorWasHiddenInGame = pOwner->IsHidden();
	pOwner->SetActorHiddenInGame(true);
	bActorHadTickEnabled = pOwner->PrimaryActorTick.bCanEverTick;
	pOwner->SetActorTickEnabled(false);

	TArray<UActorComponent*> OwnerComponents;
	pOwner->GetComponents(OwnerComponents, true);

	// Save the state for all the owner's components (collected recursively) and disable them
	for (const auto& Component : OwnerComponents)
	{
		if (Component == this)
			continue;

		FDisabledComponentInfo ComponentData{ Component, Component->PrimaryComponentTick.bCanEverTick };
		Component->PrimaryComponentTick.bCanEverTick = false;
		Component->PrimaryComponentTick.UnRegisterTickFunction();

		// Disable collision checks for any primitive. They are executed regardless of the tick state!
		if (const auto PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
		{
			ComponentData.CollisionType = PrimitiveComponent->GetCollisionEnabled();
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			//TODO: Maybe we need to SetGenerateOverlapEvents(false) too?
		}
		DisabledComponentsData.Add(ComponentData);
	}

	bIsActive = false;
}

void UMIsActiveCheckerComponent::Enable()
{
	const auto pOwner = GetOwner();
	if (!pOwner)
	{
		check(false);
		return;
	}

	pOwner->SetActorHiddenInGame(bActorWasHiddenInGame);
	pOwner->SetActorTickEnabled(bActorHadTickEnabled);

	// Set the components state using saved data
	for (auto& Data : DisabledComponentsData)
	{
		if (Data.bCanEverTick)
		{
			Data.Component->PrimaryComponentTick.bCanEverTick = true;

			// Register tick function
			if (Data.Component->SetupActorComponentTickFunction(&Data.Component->PrimaryComponentTick))
			{
				Data.Component->PrimaryComponentTick.Target = Data.Component;
			}

			Data.Component->PrimaryComponentTick.SetTickFunctionEnable(true);
		}

		if (const auto PrimitiveComponent = Cast<UPrimitiveComponent>(Data.Component))
		{
			PrimitiveComponent->SetCollisionEnabled(Data.CollisionType.Get(ECollisionEnabled::NoCollision));
		}
	}
	DisabledComponentsData.Empty();

	bIsActive = true;
}

void UMIsActiveCheckerComponent::SetUpCollisionPrimitive()
{
	const auto pOwner = GetOwner();
	if (!pOwner)
	{
		check(false);
		return;
	}
	
	auto MarkedComponents = pOwner->GetComponentsByTag(UPrimitiveComponent::StaticClass(), "IsActiveChecker");
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
	CollisionPrimitive->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionPrimitive->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	CollisionPrimitive->SetGenerateOverlapEvents(false);
}
