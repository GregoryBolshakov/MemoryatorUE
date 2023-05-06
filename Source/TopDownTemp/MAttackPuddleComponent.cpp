// Copyright Epic Games, Inc. All Rights Reserved.

#include "MAttackPuddleComponent.h"

#include "M2DRepresentationBlueprintLibrary.h"
#include "M2DRepresentationComponent.h"
#include "MCharacter.h"
#include "Components/BoxComponent.h"

UMAttackPuddleComponent::UMAttackPuddleComponent()
	: DynamicMaterialInterface(nullptr), DynamicMaterial(nullptr)
	, Angle(0)
	, Length(0)
{
	// Collision setup
	UPrimitiveComponent::SetCollisionObjectType(ECC_WorldDynamic);
	UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	UPrimitiveComponent::SetCollisionResponseToAllChannels(ECR_Ignore);
	UPrimitiveComponent::SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);

	bCanEverAffectNavigation = false;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

#ifdef WITH_EDITOR
	SetVisibleFlag(false);
#endif
}

void UMAttackPuddleComponent::BeginPlay()
{
	Super::BeginPlay();

	if (DynamicMaterialInterface)
	{
		DynamicMaterial = CreateDynamicMaterialInstance(0, DynamicMaterialInterface);

#ifdef WITH_EDITOR
		if (!bHiddenInGame)
		{
			SetVisibleFlag(true);
		}
		pPerimeterOutline->SetVisibleFlag(true);
	}
#endif
}

void UMAttackPuddleComponent::SetLength(float IN_Length)
{
	Length = IN_Length;
	if (!SourceSprite || !DynamicMaterialInterface)
		return;

	const auto PuddleBounds = CalcBounds(GetComponentTransform());
	auto PuddleScale = GetComponentScale();
	PuddleScale.X *= IN_Length / PuddleBounds.BoxExtent.X;
	PuddleScale.Z *= IN_Length / PuddleBounds.BoxExtent.Z;
	SetWorldScale3D(PuddleScale);

	// Configure perimeter outline size
	if (const auto OwnerCharacter = Cast<AMCharacter>(GetOwner()))
	{
		auto PerimeterOutlineScale = pPerimeterOutline->GetComponentScale();
		PerimeterOutlineScale.X *= OwnerCharacter->GetRadius() / pPerimeterOutline->Bounds.BoxExtent.X;
		PerimeterOutlineScale.Z *= OwnerCharacter->GetRadius() / pPerimeterOutline->Bounds.BoxExtent.Z;
		pPerimeterOutline->SetWorldScale3D(PerimeterOutlineScale);
	}
}

bool UMAttackPuddleComponent::IsCircleWithin(const FVector& Center, float Radius) const
{
	const auto MyBounds = CalcBounds(GetComponentTransform());
	check(FMath::IsNearlyEqual(MyBounds.BoxExtent.X, MyBounds.BoxExtent.Y, 0.001));

	auto MyRotation = FRotationMatrix::MakeFromX(GetForwardVector()).Rotator().Yaw - 90.f; // we always add 90 for 2D objects because they are arranged along the x-axis, not across

	FVector Point1, Point2; // The left and right-most intersection points of the circle O1, R1 = [O2-O1] and circle O2, R2 = object's radius 
	const auto ToPointVector = Center - GetComponentLocation();
	FindIntersectionPoints(GetComponentLocation(), ToPointVector.Size2D(), Center, Radius, Point1, Point2);

	// Rotations to the left-most and right-most edges of checkable circle 
	const auto LeftEdgeRotation = FRotationMatrix::MakeFromX(Point1 - GetComponentLocation()).Rotator().Yaw;
	const auto RightEdgeRotation = FRotationMatrix::MakeFromX(Point2 - GetComponentLocation()).Rotator().Yaw;

	const auto ToPointRotation = FRotationMatrix::MakeFromX(ToPointVector).Rotator().Yaw;
	const auto MinSufficientAngle = abs(FMath::FindDeltaAngleDegrees(LeftEdgeRotation, RightEdgeRotation) / 2.f);

	// Angle between rotations we just found to the boundaries of the segment 
	const auto LeftEdgeToBorderAngle = abs(FMath::FindDeltaAngleDegrees(MyRotation, ToPointRotation));
	const auto RightEdgeToBorderAngle = abs(FMath::FindDeltaAngleDegrees(MyRotation + Angle, ToPointRotation));

	if (Angle <= 180.f && (LeftEdgeToBorderAngle <= Angle + MinSufficientAngle && RightEdgeToBorderAngle <= Angle + MinSufficientAngle))
	{
		return true;
	}

	const float InvertedAngle = 360.f - Angle;
	if (Angle > 180.f && (LeftEdgeToBorderAngle >= InvertedAngle - MinSufficientAngle || RightEdgeToBorderAngle >= InvertedAngle - MinSufficientAngle))
	{
		return true;
	}

	return false;
}

void UMAttackPuddleComponent::SetAngle(float Value)
{
	Value -= (static_cast<int>(Value) / 360) * 360.f;
	Angle = Value;
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue("Alpha", Value / 360.f);
	}
}

void UMAttackPuddleComponent::UpdateRotation()
{
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

void UMAttackPuddleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateRotation();
}

void UMAttackPuddleComponent::FindIntersectionPoints(FVector O1, float r1, FVector O2, float r2, FVector& point1, FVector& point2) const
{
	float d = sqrt(pow(O2.X - O1.X, 2) + pow(O2.Y - O1.Y, 2));

	// Check that two circles actually have 2 intersection points
	if (d > r1 + r2 || d < abs(r1 - r2) || (d == 0 && r1 == r2)) {
		check(false);
		return;
	}

	float a = (pow(r1, 2) - pow(r2, 2) + pow(d, 2)) / (2 * d);
	float h = sqrt(pow(r1, 2) - pow(a, 2));

	float x3 = O1.X + a * (O2.X - O1.X) / d;
	float y3 = O1.Y + a * (O2.Y - O1.Y) / d;

	point1.X = x3 + h * (O2.Y - O1.Y) / d;
	point1.Y = y3 - h * (O2.X - O1.X) / d;

	point2.X = x3 - h * (O2.Y - O1.Y) / d;
	point2.Y = y3 + h * (O2.X - O1.X) / d;
}
