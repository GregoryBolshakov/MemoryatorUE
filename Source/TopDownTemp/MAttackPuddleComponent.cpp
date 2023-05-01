// Copyright Epic Games, Inc. All Rights Reserved.

#include "MAttackPuddleComponent.h"

#include "M2DRepresentationBlueprintLibrary.h"
#include "M2DRepresentationComponent.h"

UMAttackPuddleComponent::UMAttackPuddleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DynamicMaterialInterface(nullptr)
	, DynamicMaterial(nullptr)
	, Angle(0)
{
}

void UMAttackPuddleComponent::BeginPlay()
{
	Super::BeginPlay();

	if (DynamicMaterialInterface)
	{
		DynamicMaterial = CreateDynamicMaterialInstance(0, DynamicMaterialInterface);
	}
}

void UMAttackPuddleComponent::SetLength(float Length)
{
	if (!SourceSprite || !DynamicMaterialInterface)
		return;

	const auto PuddleBounds = CalcBounds(GetComponentTransform());
	auto PuddleScale = GetComponentScale();
	PuddleScale.X *= Length / PuddleBounds.BoxExtent.X;
	PuddleScale.Z *= Length / PuddleBounds.BoxExtent.Y;
	SetWorldScale3D(PuddleScale);
}

bool UMAttackPuddleComponent::IsPointWithin(FVector Point)
{
	const auto MyBounds = CalcBounds(GetComponentTransform());
	check(FMath::IsNearlyEqual(MyBounds.BoxExtent.X, MyBounds.BoxExtent.Y));
	const auto MyRadius = MyBounds.BoxExtent.X;
	if (FVector::Dist2D(GetComponentLocation(), Point) > MyRadius) // Check if inside the circle
		return false;

	const auto ToPointVector = Point - GetComponentLocation();
	auto RotationToPoint = FRotationMatrix::MakeFromX(ToPointVector).Rotator().Yaw;
	RotationToPoint = RotationToPoint < 0.f ? RotationToPoint + 360.f : RotationToPoint; // Make sure angle is [0, 380]
	auto MyRotation = FRotationMatrix::MakeFromX(GetForwardVector()).Rotator().Yaw - 90.f; // we always add 90 for 2D objects because they are arranged along the x-axis, not across
	MyRotation = MyRotation < 0.f ? MyRotation + 360.f : MyRotation; // Make sure angle is [0, 380]

	 return MyRotation <= RotationToPoint && MyRotation + Angle >= RotationToPoint;
}

void UMAttackPuddleComponent::SetAngle(float Value)
{
	Value -= (static_cast<int>(Value) / 360) * 360.f;
	Angle = Value;
	DynamicMaterial->SetScalarParameterValue("Alpha", Value / 360.f);
}

void UMAttackPuddleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (const auto OwnerActor = GetOwner())
	{
		if (const auto RepresentationComponent = OwnerActor->FindComponentByClass<UM2DRepresentationComponent>())
		{
			SetWorldRotation(FRotator(0.f, 90.f, -90.f)); // Rotate to lay on the ground
			AddWorldRotation(FRotator(0.f, FRotationMatrix::MakeFromX(RepresentationComponent->LastValidGaze).Rotator().Yaw, 0.f)); // Actual rotation
			AddWorldRotation(FRotator(0.f, Angle / -2.f, 0.f)); // Shift to face the the segment center
		}
	}
}
