// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MActor.generated.h"

UCLASS()
class TOPDOWNTEMP_API AMActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:

	virtual void PostInitializeComponents() override;

protected:

	UPROPERTY(Category=Representation, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) 
	class UM2DRepresentationComponent* RepresentationComponent;

	UPROPERTY(Category=ActiveCheck, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMIsActiveCheckerComponent* IsActiveCheckerComponent;
};