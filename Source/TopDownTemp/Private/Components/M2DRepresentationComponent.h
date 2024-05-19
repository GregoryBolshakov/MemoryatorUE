#pragma once

#include "CoreMinimal.h"
#include "M2DRepresentationComponent.generated.h"

class UCapsuleComponent;
class UM2DShadowControllerComponent;

/** Component that rotates all attached UMeshComponents towards the first local camera.
 *
 * @ Don't attach any mesh to another mesh.
 * 
 * @ Each mesh can have one attached USceneComponent acting as the pivot point. If none is present, it uses mesh's location.
 * 
 * @ Each mesh determines its own flipped state (Z-axis mirroring)
 *
 *  Right now it is responsible for creating shadow twins, but it is subject to change.
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="Component that rotates all attached UMeshComponents towards the first local camera."))
class TOPDOWNTEMP_API UM2DRepresentationComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:
	const TArray<UCapsuleComponent*>& GetCapsuleComponentArray() { return CapsuleComponentArray; }

	/** Should be called in owner's PostInitializeComponents */
	void PostInitChildren();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetMeshByGazeAndVelocity(const FVector& IN_Gaze, const FVector& IN_Velocity, const FName& Tag = "");

	UFUNCTION(BlueprintCallable)
	void SetColor(const FLinearColor& Color);

	UPROPERTY(Category=Rendering, EditAnywhere, BlueprintReadWrite)
	bool bFaceToCamera = true;

	UPROPERTY(Category=Rendering, EditAnywhere, BlueprintReadWrite)
	bool bCastShadow = true;

	/** Representation component is 3D object and it would be ambiguous to tell where exactly this object is tilted.
	 * This is the reason of heaving imaginary rotation of the object after it was rotated toward the camera. */
	UPROPERTY(Category=Rendering, EditAnywhere, BlueprintReadWrite)
	FRotator RotationWhileFacingCamera;

	UPROPERTY(BlueprintReadOnly, Category="MRotatableFlipbookComponent")
	FVector LastValidGaze;

private:
	void FaceToCamera();

	void SetUpSprites();

	/** Creates invisible twin-components for casting non rotatable shadows */
	void CreateShadowTwins();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void InterpolateColor(float DeltaTime);

	UPROPERTY() 
	TArray<UCapsuleComponent*> CapsuleComponentArray; // TODO: It's no longer used, should be removed

	UPROPERTY()
	TArray<UMeshComponent*> RenderComponentArray;

	UPROPERTY()
	TArray<UMeshComponent*> ShadowTwinComponentArray;

	UPROPERTY()
	TArray<UM2DShadowControllerComponent*> ShadowTwinControllerArray;
	
	UPROPERTY()
	APlayerCameraManager* CameraManager;

	FLinearColor CurrentColor = FLinearColor::White;
	FLinearColor DesiredColor = FLinearColor::White;
	UPROPERTY(EditDefaultsOnly, Category=Rendering)
	float ColorChangingSpeed = 15.f;
};