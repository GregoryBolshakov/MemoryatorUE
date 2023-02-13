// Fill out your copyright notice in the Description page of Project Settings.

#include "MActor.h"

#include "M2DRepresentationComponent.h"
#include "MIsActiveCheckerComponent.h"
#include "Kismet/GameplayStatics.h"

AMActor::AMActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	RepresentationComponent = CreateDefaultSubobject<UM2DRepresentationComponent>(TEXT("Representation"));
	SetRootComponent(RepresentationComponent);

	IsActiveCheckerComponent = CreateDefaultSubobject<UMIsActiveCheckerComponent>(TEXT("IsActiveChecker"));
	auto testComp = CreateDefaultSubobject<UActorComponent>(TEXT("TesComp"));
}

void AMActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	IsActiveCheckerComponent->Disable();
	IsActiveCheckerComponent->SetUpCollisionPrimitive();
}

