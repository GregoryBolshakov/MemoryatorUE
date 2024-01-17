// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MOutpostGenerator.generated.h"

class AMOutpostHouse;
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

	float GetRadius() const { return Radius; }

protected:
	bool bGenerated = false;

	UPROPERTY(EditDefaultsOnly)
	float Radius = 1500.f;

	//TODO: Consider tracking other outpost elements
	TMap<FName, AMOutpostHouse*> Houses;
};

USTRUCT()
struct FOutpostSave
{
	GENERATED_BODY()
	TArray<FUid> Houses;
};