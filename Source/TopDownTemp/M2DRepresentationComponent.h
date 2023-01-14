#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "M2DRepresentationComponent.generated.h"

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="A component that has 2D sprites which always face the camera."))
class TOPDOWNTEMP_API UM2DRepresentationComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(Category=Rendering, EditAnywhere, BlueprintReadWrite)
	bool bFaceToCamera = true;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	virtual void BeginPlay() override;
	
	void FaceToCamera();

	UPROPERTY(Category=Shape, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) 
	TArray<class UCapsuleComponent*> CapsuleComponentArray;
	
	UPROPERTY(Category=Sprite, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TArray<class UMeshComponent*> RenderComponentArray;
	
	UPROPERTY()
	APlayerCameraManager* CameraManager;
};