// Fill out your copyright notice in the Description page of Project Settings.

#include "MBanditCampGenerator.h"

DEFINE_LOG_CATEGORY(LogBanditCampGenerator);

AMBanditCampGenerator::AMBanditCampGenerator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

FTimerHandle tempTimer3; //temp
void AMBanditCampGenerator::Generate()
{
	Super::Generate();

	SpawnOutpostElementAtLocation(CenterElement, GetActorLocation());

	GenerateOnCirclePerimeter(GetActorLocation(), CartsCircleRadius, CartsData);
}
