// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MOutpostGenerator.generated.h"

/**
 * The base class for all outpost generators. They are responsible for spawning buildings,
 * determining their, types, quantity and other specifics within one outpost.
 */
UCLASS()
class TOPDOWNTEMP_API AMOutpostGenerator : public AActor
{
	GENERATED_BODY()

public:
	virtual void Generate() { bGenerated = true; };

	bool IsGenerated() const { return bGenerated; }

protected:
	bool bGenerated = false;
};
