// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Managers/MWorldGeneratorTypes.h"
#include "MActor.generated.h"

class UMInventoryComponent;
struct FMActorSaveData;

USTRUCT(BlueprintType)
struct FArrayMaterialInstanceDynamicWrapper
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> ArrayMaterialInstanceDynamic;

	void Add(UMaterialInstanceDynamic* Item) { ArrayMaterialInstanceDynamic.Add(Item); }
};

/** Stationary actor that may be flat, face camera and cast artificial shadows */
UCLASS()
class TOPDOWNTEMP_API AMActor : public AActor
{
	GENERATED_UCLASS_BODY()

public: // Have effect only when called under FOnSpawnActorStarted

	UFUNCTION(BlueprintCallable)
	void SetAppearanceID(int IN_AppearanceID) { AppearanceID = IN_AppearanceID; }

public:
	/** Use AMWorldGenerator::RemoveFromGrid instead */
	//bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true ) = delete;

	/** Never destroy objects within the grid using AActor::Destroy. Identical naming is used to minimize calls to the wrong Destroy().
	 *  But we are still not immune from incorrect calls if the pointer to a MActor or MCharacter is of type AActor */
	bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true);

	UMInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintCallable)
	FMUid GetUid() const { return Uid; }

	int GetAppearanceID() const { return AppearanceID; }
	bool GetIsRandomizedAppearance() const { return IsRandomizedAppearance; }

	void SetUid(const FMUid& _Uid) { Uid = _Uid; }

	UFUNCTION(BlueprintCallable)
	void InitialiseInventory(const TArray<struct FItem>& IN_Items);

	UFUNCTION(BlueprintCallable)
	const TMap<UStaticMeshComponent*, FArrayMaterialInstanceDynamicWrapper>& GetDynamicMaterials() const
	{
		return DynamicMaterials;
	}

	/** Start from the base and compose structs upwards (FActorSaveData -> might add more in between -> FMActorSaveData).\n
	 * Must always call Super::GetSaveData(). */
	virtual FMActorSaveData GetSaveData() const;

	virtual void BeginLoadFromSD(const FMActorSaveData& MActorSD);

protected:

	virtual void PostInitializeComponents() override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly) 
	class UM2DRepresentationComponent* FaceCameraComponent;

	UPROPERTY(Category=ActiveCheck, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMIsActiveCheckerComponent* IsActiveCheckerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UMInventoryComponent* InventoryComponent;

	// I don't like having it both here and in the metadata. But client needs it to do requests
	UPROPERTY(Replicated)
	FMUid Uid;

	/** Objects like trees/plants/mushrooms/stones/etc. may have different appearances. If true, the type will be picked randomly */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool IsRandomizedAppearance = false;

	UPROPERTY(BlueprintReadOnly)
	int AppearanceID = 0;

	UPROPERTY()
	TMap<UStaticMeshComponent*, FArrayMaterialInstanceDynamicWrapper> DynamicMaterials;
};