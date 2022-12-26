// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MWorldGenerator.generated.h"

class AMGroundBlock;
class AMTree;

/**
 * 
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API UMWorldGenerator : public UObject
{
	GENERATED_BODY()
public:

	void GenerateWorld();

	template<typename T>
	static FString GetStringByClass();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AMGroundBlock> ToSpawnGroundBlock;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AMActor> ToSpawnTree;

private:

	UPROPERTY(EditAnywhere)
	FVector2D WorldSize{10000, 10000};

	UPROPERTY()
	TMap<FName, AActor*> Objects;
};

template <typename T>
FString UMWorldGenerator::GetStringByClass()
{
	if (TIsSame<T, AMGroundBlock>::Value)
		return "GroundBlock";
	return "unknown";
}