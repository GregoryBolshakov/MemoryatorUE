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

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetMeshByRotation(float Angle, const FName& Tag = "");

	static float GetCameraDeflectionAngle(const UObject* WorldContextObject, const FVector Location, const FVector GazeDirection);

	UPROPERTY(Category=Rendering, EditAnywhere, BlueprintReadWrite)
	bool bFaceToCamera = true;

private:

	void FaceToCamera();

	void SetUpSprites();

	UPROPERTY() 
	TArray<class UCapsuleComponent*> CapsuleComponentArray;

	UPROPERTY()
	TArray<class UMeshComponent*> RenderComponentArray;
	
	UPROPERTY()
	APlayerCameraManager* CameraManager;
};