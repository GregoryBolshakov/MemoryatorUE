// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperSpriteComponent.h"
#include "MAttackPuddleComponent.generated.h"

/** Component represents the zone of the attack */
UCLASS(BlueprintType, Blueprintable, HideCategories=(Materials))
class TOPDOWNTEMP_API UMAttackPuddleComponent : public UPaperSpriteComponent
{
	//TODO: Hide the Sprite category, because there's no difference which to use.
	GENERATED_UCLASS_BODY()

	virtual void BeginPlay() override;

	void SetLength(float Length);

	bool IsPointWithin(FVector Point);

protected:

	/** Sets the progress between 0 and 360.f */
	UFUNCTION(BlueprintCallable)
	void SetAngle(float Value);

	UFUNCTION(BlueprintCallable)
	float GetProgress() const { return Angle; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, Category=MAttackPuddleComponent)
	UMaterialInterface* DynamicMaterialInterface;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	float Angle;

};
