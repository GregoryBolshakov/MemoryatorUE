// Fill out your copyright notice in the Description page of Project Settings.


#include "MGroundBlock.h"

#include "M2DRepresentationComponent.h"
#include "MIsActiveCheckerComponent.h"

void AMGroundBlock::SetComponentSize(UPrimitiveComponent& Component, FVector Size)
{
	const auto CurrentSize = Component.GetComponentTransform().TransformVector(Component.Bounds.GetBox().GetSize());
	const auto CurrentScale = Component.GetComponentScale();
	const auto NewScale = CurrentScale * (Size / CurrentSize);
	Component.SetRelativeScale3D(NewScale);
}

void AMGroundBlock::AlignComponentSize() const
{
	// Scale and rotate the mesh according to the Size value
	TArray<USceneComponent*> ChildComponents;
	RepresentationComponent->GetChildrenComponents(false, ChildComponents);

	for (const auto& ChildComponent : ChildComponents)
	{
		if (const auto ChildMesh = Cast<UMeshComponent>(ChildComponent))
		{
			SetComponentSize(*ChildMesh, Size);
		}
	}

	// Resize collision primitive for IsActive check
	if (IsActiveCheckerComponent && IsActiveCheckerComponent->GetPrimitive())
	{
		SetComponentSize(*IsActiveCheckerComponent->GetPrimitive(), FVector(Size.X, Size.Z, 1.f));
	}
}

void AMGroundBlock::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AlignComponentSize();
}


