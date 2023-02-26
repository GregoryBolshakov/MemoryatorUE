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

	/** Should be called in owner's PostInitializeComponents */
	void PostInitChildren();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetMeshByRotation(float Angle, const FName& Tag = "");

	static float GetCameraDeflectionAngle(const UObject* WorldContextObject, const FVector GazeDirection);

	UPROPERTY(Category=Rendering, EditAnywhere, BlueprintReadWrite)
	bool bFaceToCamera = true;

private:

	void FaceToCamera();

	void SetUpSprites();

	/** Adds invisible twin-components for casting non rotatable shadows */
	void CreateShadowTwins();

	UPROPERTY() 
	TArray<class UCapsuleComponent*> CapsuleComponentArray;

	UPROPERTY()
	TArray<UMeshComponent*> RenderComponentArray;

	UPROPERTY()
	TArray<UMeshComponent*> ShadowTwinComponentArray;
	
	UPROPERTY()
	APlayerCameraManager* CameraManager;
};