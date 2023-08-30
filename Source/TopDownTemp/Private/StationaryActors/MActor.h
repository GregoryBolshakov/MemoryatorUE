// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Managers/MWorldGeneratorTypes.h"
#include "MActor.generated.h"

UCLASS()
class TOPDOWNTEMP_API AMActor : public AActor
{
	GENERATED_UCLASS_BODY()

public: // Have effect only when called under FOnSpawnActorStarted

	void SetBiome(EBiome IN_Biome) { Biome = IN_Biome; }
	void SetAppearanceID(int IN_AppearanceID) { AppearanceID = IN_AppearanceID; }
	void MakeRandom() { IsRandomizedAppearance = true; }

public:

	EBiome GetBiome() const { return Biome; }
	int GetAppearanceID() const { return AppearanceID; }
	bool GetIsRandomizedAppearance() const { return IsRandomizedAppearance; }

protected:

	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintImplementableEvent)
	void Randomize(EBiome IN_Biome);

	UPROPERTY()
	USceneComponent* PointComponent;

	UPROPERTY(Category=Representation, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) 
	class UM2DRepresentationComponent* RepresentationComponent;

	UPROPERTY(Category=ActiveCheck, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMIsActiveCheckerComponent* IsActiveCheckerComponent;

	/** Objects like trees/plants/mushrooms/stones/etc. may have different appearances. If true, the type will be picked randomly */
	bool IsRandomizedAppearance = false;

	EBiome Biome;
	int AppearanceID;
};