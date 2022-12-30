// Fill out your copyright notice in the Description page of Project Settings.

#include "MActor.h"

#include "M2DRepresentationComponent.h"
#include "Kismet/GameplayStatics.h"

AMActor::AMActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	RepresentationComponent = CreateDefaultSubobject<UM2DRepresentationComponent>(TEXT("RepresentationComponent"));
	SetRootComponent(RepresentationComponent);
}

void AMActor::BeginPlay()
{
	Super::BeginPlay();
}

/*#if WITH_EDITOR
bool AMActor::GetReferencedContentObjects(TArray<UObject*>& Objects) const
{
	Super::GetReferencedContentObjects(Objects);

	for (const auto& RenderComponent : RenderComponentArray)
	{
		if (UPaperSprite* SourceSprite = RenderComponent->GetSprite())
		{
			Objects.Add(SourceSprite);
		}
	}
	return true;
}
#endif*/
