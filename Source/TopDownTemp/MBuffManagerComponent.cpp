#include "MBuffManagerComponent.h"

#include "M2DRepresentationComponent.h"
#include "MBuffBarWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"

UMBuffManagerComponent::UMBuffManagerComponent()
{
	// Create delegates for all buff types
	if (const UEnum* EnumClass = StaticEnum<EBuffType>())
	{
		for (int32 i = 0; i < EnumClass->NumEnums() - 1; ++i) // Subtracting 1 to avoid _MAX value
		{
			EBuffType EnumValue = static_cast<EBuffType>(EnumClass->GetValueByIndex(i));
			BuffDelegates.Add(EnumValue, {});
		}
	}

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMBuffManagerComponent::AddBuff(EBuffType Type, float Duration)
{
	// Find buff if exists or create new otherwise
	FBuff* Buff = ActiveBuffs.Find(Type);
	if (!Buff)
	{
		Buff = &ActiveBuffs.Add(Type, {});
	}

	Buff->Stack++;

	// Calculate the new duration
	auto TimeRemain = FMath::Max(0.f, GetWorld()->GetTimerManager().GetTimerRemaining(Buff->TimerHandle));
	TimeRemain = FMath::Max(TimeRemain, Duration); // We might consider adding time instead

	// To extend the duration, we need to clear the current timer and set a new one with a longer time
	GetWorld()->GetTimerManager().ClearTimer(Buff->TimerHandle);
	GetWorld()->GetTimerManager().SetTimer(Buff->TimerHandle, [this, Buff, Type]
	{
		BuffDelegates[Type].ExecuteIfBound(Buff->Stack);
		ActiveBuffs.Remove(Type);
	}, TimeRemain, false);

	if (const auto BuffBarWidget = Cast<UMBuffBarWidget>(BuffBarWidgetComponent->GetWidget()))
	{
		BuffBarWidget->AddBuff(Type);
	}
}

bool UMBuffManagerComponent::IsBuffSet(EBuffType Type)
{
	if (const auto Buff = ActiveBuffs.Find(Type))
	{
		if (GetWorld()->GetTimerManager().IsTimerActive(Buff->TimerHandle) && Buff->Stack > 0)
		{
			return true;
		}
		check(false);
		GetWorld()->GetTimerManager().ClearTimer(Buff->TimerHandle);
		return false;
	}

	return false;
}

void UMBuffManagerComponent::CreateWidget()
{
	const auto pOwner = GetOwner();
	if (!pOwner)
		return;

	const auto RepresentationComponent = pOwner->FindComponentByClass<UM2DRepresentationComponent>();
	if (!RepresentationComponent)
		return;

	BuffBarWidgetComponent = NewObject<UWidgetComponent>(this, UWidgetComponent::StaticClass());
	BuffBarWidgetComponent->SetWidgetClass(BuffBarWidgetBPClass);
	BuffBarWidgetComponent->CreationMethod = EComponentCreationMethod::Instance;
	BuffBarWidgetComponent->RegisterComponent();
	BuffBarWidgetComponent->AttachToComponent(RepresentationComponent, FAttachmentTransformRules::KeepRelativeTransform);

	BuffBarWidgetComponent->SetTwoSided(false);
	BuffBarWidgetComponent->CastShadow = false;
	BuffBarWidgetComponent->SetDrawSize({90.f, 90.f});
}

void UMBuffManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	BuffBarWidgetComponent->UpdateChildTransforms();
	BuffBarWidgetComponent->UpdateBounds();

	auto test0 = BuffBarWidgetComponent->GetWidget()->GetDesiredSize();
	auto test1 = BuffBarWidgetComponent->GetDrawSize();
	auto test = BuffBarWidgetComponent->CalcBounds(BuffBarWidgetComponent->GetComponentTransform());
	if (const auto pOwner = GetOwner())
	{
		const auto OwnerCapsule = pOwner->FindComponentByClass<UCapsuleComponent>();
		const auto TopCapsulePoint = OwnerCapsule->GetComponentLocation() + FVector(0.f, 0.f, OwnerCapsule->GetScaledCapsuleHalfHeight());
		const auto NewLocation = TopCapsulePoint + FVector(0.f, 0.f, BuffBarWidgetComponent->CalcBounds(BuffBarWidgetComponent->GetComponentTransform()).BoxExtent.Z / 2.f);
		BuffBarWidgetComponent->SetWorldLocation(NewLocation);
	}
}
