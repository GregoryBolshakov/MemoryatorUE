// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MActor.h"
#include "Managers/MWorldGeneratorTypes.h"
#include "MGroundBlock.generated.h"

class UPaperSpriteComponent;
/**
 * 
 */
UCLASS()
class TOPDOWNTEMP_API AMGroundBlock : public AMActor
{
	GENERATED_UCLASS_BODY()

public:

	/** Get the biome from AMWorldGenerator::GetGridOfActors.
	 * We can't rely on storing biome here because there is a gap between biome setting pass and block generating pass.
	 * During generating pass each block gets its biome in turn so there would be a risk of adjacent block store an old biome */
	UFUNCTION(BlueprintCallable)
	void UpdateBiome(EBiome IN_Biome);

	/** Function updates the transition if needed. Called by an adjacent block that just received its biome. */
	void UpdateTransition(FIntPoint Offset, EBiome AdjacentBiome);

protected:

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnBiomeUpdated();

	UStaticMeshComponent* GetTransitionByOffset(FIntPoint Offset) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* GroundMeshComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* GroundTransitionMeshLeft;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* GroundTransitionMeshRight;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* GroundTransitionMeshTop;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* GroundTransitionMeshBottom;

// PCG
public:
	UPROPERTY(ReplicatedUsing=OnPCGVariablesReplicated, VisibleAnywhere, BlueprintReadOnly)
	FPCGVariables PCGVariables = {};

	UFUNCTION()
	void OnPCGVariablesReplicated();
};
