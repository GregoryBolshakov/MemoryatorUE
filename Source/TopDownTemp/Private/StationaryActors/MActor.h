// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Managers/MWorldGeneratorTypes.h"
#include "MActor.generated.h"

/** Stationary actor that may be flat, face camera and cast artificial shadows */
UCLASS()
class TOPDOWNTEMP_API AMActor : public AActor
{
	GENERATED_UCLASS_BODY()

public: // Have effect only when called under FOnSpawnActorStarted

	UFUNCTION(BlueprintCallable)
	void SetAppearanceID(int IN_AppearanceID) { AppearanceID = IN_AppearanceID; }
	void MakeRandom() { IsRandomizedAppearance = true; }

public:
	/** Use AMWorldGenerator::RemoveFromGrid instead */
	//bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true ) = delete;

	// Temp workaraund
	bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true);

	int GetAppearanceID() const { return AppearanceID; }
	bool GetIsRandomizedAppearance() const { return IsRandomizedAppearance; }

protected:

	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintImplementableEvent)
	void Randomize();

	UFUNCTION(BlueprintCallable)
	EBiome GetMyBiome();

	UPROPERTY()
	USceneComponent* PointComponent;

	UPROPERTY(Category=Representation, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) 
	class UM2DRepresentationComponent* RepresentationComponent;

	UPROPERTY(Category=ActiveCheck, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMIsActiveCheckerComponent* IsActiveCheckerComponent;

	/** Objects like trees/plants/mushrooms/stones/etc. may have different appearances. If true, the type will be picked randomly */
	bool IsRandomizedAppearance = false;

	int AppearanceID = 0;
};