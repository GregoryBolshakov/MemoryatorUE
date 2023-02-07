// Copyright Epic Games, Inc. All Rights Reserved.

#include "MMob.h"
#include "MMobController.h"

AMMob::AMMob(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AMMob::HandleAnimationStates()
{
	Super::HandleAnimationStates();

	const auto pController = Cast<AMMobController>(GetController());
	if (!pController)
	{
		check(false);
		return;
	}
}
