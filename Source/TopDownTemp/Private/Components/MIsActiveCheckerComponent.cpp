#include "MIsActiveCheckerComponent.h"

#include "Components/PrimitiveComponent.h"

UMIsActiveCheckerComponent::UMIsActiveCheckerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMIsActiveCheckerComponent::DisableOwner(bool bForce)
{
	// No need to disable if either already disabled or configured to always be enabled
	if (!bIsOwnerActive || bAlwaysEnabled)
	{
		return;
	}

	bIsOwnerDisabledByForce = bForce;

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

		FDisabledComponentInfo ComponentData{ Component, static_cast<bool>(Component->PrimaryComponentTick.bCanEverTick) };
		Component->PrimaryComponentTick.bCanEverTick = false;
		Component->PrimaryComponentTick.UnRegisterTickFunction();

		//TODO: Check if we need to SetGenerateOverlapEvents(false)

		DisabledComponentsData.Add(ComponentData);
	}

	bIsOwnerActive = false;
	//TODO: Disable actor's controller if present

	OnDisabledDelegate.Execute();
}

void UMIsActiveCheckerComponent::EnableOwner(bool bForce)
{
	// No need to enable if bAlwaysEnabled is true, because it has never been disabled.
	// If was disabled by force, then can be enabled only by force
	if (bIsOwnerActive || bAlwaysEnabled || (bIsOwnerDisabledByForce && !bForce))
	{
		return;
	}

	const auto pOwner = GetOwner();
	if (!pOwner)
	{
		check(false);
		return;
	}

	if (bActorWasHiddenInGame.IsSet())
	{
		pOwner->SetActorHiddenInGame(bActorWasHiddenInGame.GetValue());
	}
	if (bActorHadTickEnabled.IsSet())
	{
		pOwner->SetActorTickEnabled(bActorHadTickEnabled.GetValue());
	}

	// Set the components state using saved data
	for (auto& Data : DisabledComponentsData)
	{
		if (!Data.Component)
		{
			check(false);
			continue;
		}
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
	}
	DisabledComponentsData.Empty();

	bIsOwnerActive = true;
	//TODO: Enable actor's controller if present

	OnEnabledDelegate.Execute();
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

	// For now it's useless
	CollisionPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionPrimitive->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionPrimitive->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	//CollisionPrimitive->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	//CollisionPrimitive->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionPrimitive->SetGenerateOverlapEvents(false);
}
