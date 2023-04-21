// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MActor.h"
#include "MWorldGeneratorTypes.h"
#include "MGroundBlock.generated.h"

/**
 * 
 */
UCLASS()
class TOPDOWNTEMP_API AMGroundBlock : public AMActor
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent)
	void SetBiome(EBiome IN_Biome);
};
