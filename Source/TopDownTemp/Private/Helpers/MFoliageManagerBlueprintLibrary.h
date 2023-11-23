#pragma once

#include "CoreMinimal.h"
#include "FoliageInstancedStaticMeshComponent.h"

#include "MFoliageManagerBlueprintLibrary.generated.h"

USTRUCT(BlueprintType)
struct FIntArrayWrapper
{
//public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int> IntArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int temp = 0;
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

USTRUCT(BlueprintType)
struct FGrassChunkInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TArray<int> Indexes;

	UPROPERTY(BlueprintReadWrite)
	UFoliageInstancedStaticMeshComponent* FoliageComponent;
};


/*UCLASS()
class TOPDOWNTEMP_API UMFoliageManagerBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
};*/
