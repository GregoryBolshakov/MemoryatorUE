#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "MFoliageHelper.generated.h"

USTRUCT(BlueprintType)
struct FGrassSpawnAreaGeometry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FVector> V;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<int> Triangles;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float AreaHeight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FVector> V_Celling;
};

UCLASS(BlueprintType)
class UGrassSpawnAreaGeometryDataAsset : public UDataAsset
{
public:
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGrassSpawnAreaGeometry> Presets;
};

UCLASS(BlueprintType)
class UIntegerArrayWrapper : public UObject
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int> IntArray;

	UFUNCTION(BlueprintCallable, Category="IntegerArrayOperations")
	void AddElement(int Element) { IntArray.Add(Element); }
};
