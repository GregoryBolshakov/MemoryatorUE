// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MWorldGeneratorTypes.h"
#include "MActor.generated.h"

UCLASS()
class TOPDOWNTEMP_API AMActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:

	void SetBiomeForRandomization(EBiome Biome) { BiomeForRandomization = Biome; }

protected:

	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintImplementableEvent)
	void Randomize(EBiome Biome);

	UPROPERTY()
	USceneComponent* PointComponent;

	UPROPERTY(Category=Representation, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) 
	class UM2DRepresentationComponent* RepresentationComponent;

	UPROPERTY(Category=ActiveCheck, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMIsActiveCheckerComponent* IsActiveCheckerComponent;

	/** Objects like trees/plants/mushrooms/stones/etc. may have different appearances. If true, the type will be picked randomly */
	TOptional<EBiome> BiomeForRandomization;
};