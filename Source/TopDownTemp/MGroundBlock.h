// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MActor.h"
#include "MWorldGeneratorTypes.h"
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
	void UpdateBiome(EBiome IN_Biome);

	/** Function updates the transition if needed. Called by an adjacent block that just received its biome. */
	void UpdateTransition(FIntPoint Offset, EBiome AdjacentBiome);

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void OnBiomeUpdated();

	UFUNCTION(BlueprintCallable)
	EBiome GetMyBiome();

	UPaperSpriteComponent* GetTransitionByOffset(FIntPoint Offset) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UPaperSpriteComponent* GroundSpriteComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UPaperSpriteComponent* GroundTransitionSpriteLeft;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UPaperSpriteComponent* GroundTransitionSpriteRight;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UPaperSpriteComponent* GroundTransitionSpriteTop;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UPaperSpriteComponent* GroundTransitionSpriteBottom;
};
