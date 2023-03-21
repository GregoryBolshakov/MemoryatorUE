#pragma once

#include "CoreMinimal.h"
#include "M2DRepresentationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReverseMovementStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReverseMovementStopped);

class UCapsuleComponent;
class UM2DShadowControllerComponent;

/** Collection of 2D sprites or flipbooks that automatically face the camera.
 *
 * @ Don't attach any mesh to another mesh.
 * 
 * @ Each mesh might have one origin point (attached USceneComponent) which acts as the pivot point. If none is present, it uses its location.
 * 
 * @ Each mesh determines its own flipped state (Z-axis mirroring)
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="A component that has 2D sprites which always face the camera."))
class TOPDOWNTEMP_API UM2DRepresentationComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:

	/** Should be called in owner's PostInitializeComponents */
	void PostInitChildren();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetMeshByGazeAndVelocity(const FVector& IN_Gaze, const FVector& IN_Velocity, const FName& Tag = "");

	UPROPERTY(Category=Rendering, EditAnywhere, BlueprintReadWrite)
	bool bFaceToCamera = true;

	/** Representation component is 3D object and it would be ambiguous to tell where exactly this object is tilted.
	 * This is the reason of heaving imaginary rotation of the object after it was rotated toward the camera. */
	UPROPERTY(Category=Rendering, EditAnywhere, BlueprintReadWrite)
	FRotator RotationWhileFacingCamera;

	UPROPERTY(BlueprintAssignable, Category = "MRotatableFlipbookComponent")
	FOnReverseMovementStarted OnReverseMovementStartedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "MRotatableFlipbookComponent")
	FOnReverseMovementStopped OnReverseMovementStoppedDelegate;

	UPROPERTY(BlueprintReadOnly, Category="MRotatableFlipbookComponent")
	FVector LastValidGaze;

private:

	void FaceToCamera();

	void SetUpSprites();

	/** Creates invisible twin-components for casting non rotatable shadows */
	void CreateShadowTwins();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	UPROPERTY() 
	TArray<UCapsuleComponent*> CapsuleComponentArray;

	UPROPERTY()
	TArray<UMeshComponent*> RenderComponentArray;

	UPROPERTY()
	TArray<UMeshComponent*> ShadowTwinComponentArray;

	UPROPERTY()
	TArray<UM2DShadowControllerComponent*> ShadowTwinControllerArray;
	
	UPROPERTY()
	APlayerCameraManager* CameraManager;
};