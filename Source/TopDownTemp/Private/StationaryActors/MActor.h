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

	/** Never destroy objects within the grid using AActor::Destroy. Identical naming is used to minimize calls to the wrong Destroy().
	 *  But we are still not immune from incorrect calls if the pointer to a MActor or MCharacter is of type AActor */
	bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true);

	int GetAppearanceID() const { return AppearanceID; }
	bool GetIsRandomizedAppearance() const { return IsRandomizedAppearance; }

	UFUNCTION(BlueprintCallable)
	const TMap<UStaticMeshComponent*, UMaterialInstanceDynamic*>& GetDynamicMaterials() const
	{
		return DynamicMaterials;
	}

protected:

	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintImplementableEvent)
	void RandomizeAppearanceID();

	UFUNCTION(BlueprintImplementableEvent)
	void ApplyAppearanceID();

	/** Each material is replaced by its dynamic version in order to be modified at runtime as needed */
	void CreateDynamicMaterials();

	UFUNCTION(BlueprintCallable)
	EBiome GetMyBiome();

	UFUNCTION(BlueprintImplementableEvent)
	void OnEnabled();

	UFUNCTION(BlueprintImplementableEvent)
	void OnDisabled();

	UPROPERTY()
	USceneComponent* PointComponent;

	UPROPERTY(Category=Representation, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) 
	class UM2DRepresentationComponent* RepresentationComponent;

	UPROPERTY(Category=ActiveCheck, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMIsActiveCheckerComponent* IsActiveCheckerComponent;

	/** Objects like trees/plants/mushrooms/stones/etc. may have different appearances. If true, the type will be picked randomly */
	bool IsRandomizedAppearance = false;

	UPROPERTY(BlueprintReadOnly)
	int AppearanceID = 0;

	UPROPERTY()
	TMap<UStaticMeshComponent*, UMaterialInstanceDynamic*> DynamicMaterials;
};