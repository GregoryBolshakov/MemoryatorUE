#include "MIsActiveCheckerComponent.h"

#include "Components/PrimitiveComponent.h"

UMIsActiveCheckerComponent::UMIsActiveCheckerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsActive(true)
	, bIsDisabledByForce(false)
	, CollisionPrimitive(nullptr)
	, bAlwaysEnabled(false)
{
}

void UMIsActiveCheckerComponent::DisableOwner(bool bForce)
{
	// No need to disable if either already disabled or configured to always be enabled
	if (!bIsActive || bAlwaysEnabled)
	{
		return;
	}

	bIsDisabledByForce = bForce;

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

		// Disable collision checks for any primitive. They are executed regardless of the tick state!
		if (const auto PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
		{
			ComponentData.CollisionType = PrimitiveComponent->GetCollisionEnabled();
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			ComponentData.bCanCastShadows = PrimitiveComponent->CastShadow;
			PrimitiveComponent->CastShadow = false;
			//TODO: Maybe we need to SetGenerateOverlapEvents(false) too?
		}

		DisabledComponentsData.Add(ComponentData);
	}

	bIsActive = false;
	//TODO: Disable actor's controller if present
}

void UMIsActiveCheckerComponent::EnableOwner(bool bForce)
{
	// No need to enable if bAlwaysEnabled is true, because it has never been disabled.
	// If was disabled by force, then can be enabled only by force
	if (bIsActive || bAlwaysEnabled || (bIsDisabledByForce && !bForce))
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

		if (const auto PrimitiveComponent = Cast<UPrimitiveComponent>(Data.Component))
		{
			PrimitiveComponent->SetCollisionEnabled(Data.CollisionType.Get(ECollisionEnabled::NoCollision));

			PrimitiveComponent->CastShadow = Data.bCanCastShadows;
		}
	}
	DisabledComponentsData.Empty();

	bIsActive = true;
	//TODO: Enable actor's controller if present
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
