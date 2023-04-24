// Fill out your copyright notice in the Description page of Project Settings.

#include "MActor.h"

#include "M2DRepresentationComponent.h"
#include "MIsActiveCheckerComponent.h"
#include "Kismet/GameplayStatics.h"

AMActor::AMActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PointComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootPoint"));
	SetRootComponent(PointComponent);

	RepresentationComponent = CreateDefaultSubobject<UM2DRepresentationComponent>(TEXT("Representation"));
	RepresentationComponent->SetupAttachment(RootComponent);

	IsActiveCheckerComponent = CreateDefaultSubobject<UMIsActiveCheckerComponent>(TEXT("IsActiveChecker"));
}

void AMActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (BiomeForRandomization.IsSet())
	{
		Randomize(BiomeForRandomization.GetValue());
	}

	RepresentationComponent->PostInitChildren();
}

