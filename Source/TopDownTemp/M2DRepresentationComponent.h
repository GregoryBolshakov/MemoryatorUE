#pragma once

#include "CoreMinimal.h"
#include "M2DRepresentationComponent.generated.h"

#define EEC_Pickable ECollisionChannel::ECC_GameTraceChannel2

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
	TArray<UCapsuleComponent*> CapsuleComponentArray;

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