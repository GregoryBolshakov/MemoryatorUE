// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MActor.h"
#include "MGroundBlock.generated.h"

/**
 * 
 */
UCLASS()
class TOPDOWNTEMP_API AMGroundBlock : public AMActor
{
	GENERATED_BODY()
public:

	static void SetComponentSize(UPrimitiveComponent& Component, FVector Size);
	
	virtual void PostInitializeComponents() override;

	FVector GetSize() const { return Size; }
private:

	/** Align real size with the Size, which is adjusted in the Editor.
	All the ground blocks have to be that size for the world generation purposes.*/
	void AlignComponentSize() const;

	/** Used in world generation. Uses X and Z because all 2d sprites stretch along the X and Z axes */
	UPROPERTY(Category=Transform, EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"));
	FVector Size = FVector(400.f, 1.f, 400.f);
};
