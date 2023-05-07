#include "MBuffBarComponent.h"

#include "M2DRepresentationComponent.h"
#include "MBuffBarWidget.h"
#include "MCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"

UMBuffBarComponent::UMBuffBarComponent()
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

	SetDrawSize({100.f, 30.f});
	SetTwoSided(false);
	CastShadow = false;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMBuffBarComponent::AddBuff(EBuffType Type, float Duration)
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
		RemoveBuff(Type);
	}, TimeRemain, false);

	if (const auto BuffBarWidget = Cast<UMBuffBarWidget>(GetWidget()))
	{
		BuffBarWidget->AddBuff(Type);
	}
}

void UMBuffBarComponent::RemoveBuff(EBuffType Type)
{
	FBuff* Buff = ActiveBuffs.Find(Type);
	if (Buff)
	{
		GetWorld()->GetTimerManager().ClearTimer(Buff->TimerHandle);
		if (const auto BuffBarWidget = Cast<UMBuffBarWidget>(GetWidget()))
		{
			BuffBarWidget->RemoveBuff(Type);
		}
		ActiveBuffs.Remove(Type);
	}
}

bool UMBuffBarComponent::IsBuffSet(EBuffType Type)
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

void UMBuffBarComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateChildTransforms();
	UpdateBounds();

	if (const auto MyWidget = GetWidget())
	{
		MyWidget->InvalidateLayoutAndVolatility();
		MyWidget->ForceLayoutPrepass();
		const auto DesiredSize = MyWidget->GetDesiredSize();
		SetDrawSize(DesiredSize);
	}

	if (const auto pOwner = GetOwner())
	{
		const auto OwnerCapsule = pOwner->FindComponentByClass<UCapsuleComponent>();
		const auto TopCapsulePoint = OwnerCapsule->GetComponentLocation() + FVector(0.f, 0.f, OwnerCapsule->GetScaledCapsuleHalfHeight());
		const auto NewLocation = TopCapsulePoint + FVector(0.f, 0.f, Bounds.BoxExtent.Z / 2.f);
		SetWorldLocation(NewLocation);
	}
}

void UMBuffBarComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const auto BuffBarWidget = Cast<UMBuffBarWidget>(GetWidget()))
	{
		BuffBarWidget->SetCharacter(Cast<AMCharacter>(GetOwner()));
	}
}
