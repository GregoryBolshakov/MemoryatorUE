// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MActor.generated.h"

UCLASS()
class TOPDOWNTEMP_API AMActor : public AActor
{
	GENERATED_UCLASS_BODY()
	
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite) 
	TObjectPtr<USceneComponent> OriginPointComponent;

	UFUNCTION()
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void Tick(float DeltaSeconds) override;

	void FaceToCamera();

#if WITH_EDITOR
	virtual bool GetReferencedContentObjects(TArray<UObject*>& Objects) const override;
#endif
	
private:

	UPROPERTY(Category=Shape, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) 
	TArray<class UCapsuleComponent*> CapsuleComponentArray;
	
	UPROPERTY(Category=Sprite, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TArray<class UPaperSpriteComponent*> RenderComponentArray;
	
	UPROPERTY()
	APlayerCameraManager *CameraManager;
};