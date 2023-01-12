// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MWorldGenerator.generated.h"

class AMGroundBlock;
class AMTree;

/**
 * 
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMWorldGenerator : public AActor
{
	GENERATED_UCLASS_BODY()
public:

	void GenerateWorld();

	virtual void Tick(float DeltaSeconds) override;

	template<typename T>
	static FString GetStringByClass();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AMGroundBlock> ToSpawnGroundBlock;

	UPROPERTY(EditAnywhere)
	
	TSubclassOf<class AMActor> ToSpawnTree;

private:

	UPROPERTY(EditAnywhere)
	FVector2D WorldSize{10000, 10000};
	
	/** The box indicating the bounds of the interaction area of the world. I.e. rendering and ticking. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger Box", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* PlayerActiveZone;

	UPROPERTY()
	TMap<FString, AActor*> ActiveActors;
};

template <typename T>
FString AMWorldGenerator::GetStringByClass()
{
	if (TIsSame<T, AMGroundBlock>::Value)
		return "GroundBlock";
	return "unknown";
}