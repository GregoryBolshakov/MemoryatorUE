// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperSpriteComponent.h"
#include "MAttackPuddleComponent.generated.h"

class UBoxComponent;
//TODO: Hide the Sprite category, because there's no difference which to use.
/** Component represents the zone of the attack */
UCLASS(BlueprintType, Blueprintable, HideCategories=(Materials))
class TOPDOWNTEMP_API UMAttackPuddleComponent : public UPaperSpriteComponent
{
	GENERATED_BODY()
	UMAttackPuddleComponent();

	virtual void BeginPlay() override;

public:

	void SetLength(float IN_Length);

	void SetPerimeterOutline(UPaperSpriteComponent* IN_PerimeterOutline) { pPerimeterOutline = IN_PerimeterOutline; }

	/** Sets the progress between 0 and 360.f */
	UFUNCTION()
	void SetAngle(float Value);

	void UpdateRotation();

	bool IsCircleWithin(const FVector& Center, float Radius) const;

	UPROPERTY()
	TMap<FName, AActor*> ActorsWithin;

protected:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	float GetProgress() const { return Angle; }

	/** Finds (up to two) intersection points of two circles */
	void FindIntersectionPoints(FVector O1, float r1, FVector O2, float r2, FVector& point1, FVector& point2) const;

	UPROPERTY(EditDefaultsOnly, Category=MAttackPuddleComponent)
	UMaterialInterface* DynamicMaterialInterface;

	UPROPERTY()
	UPaperSpriteComponent* pPerimeterOutline;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	float Angle;

	float Length;

};
