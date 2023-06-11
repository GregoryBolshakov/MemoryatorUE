#pragma once

#include "Components/SceneComponent.h"
#include "M2DShadowControllerComponent.generated.h"

class UPaperFlipbook;
class ADirectionalLight;

/** Class responsible for positioning and configuring the invisible mesh that cast shadow */
UCLASS()
class TOPDOWNTEMP_API UM2DShadowControllerComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:

	void Possess(UMeshComponent* ShadowComponentToPossess, UMeshComponent* RenderComponent);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	UFUNCTION()
	void OnPossessedMeshUpdated(
		UPaperFlipbook* Flipbook,
		float PlaybackPosition = 0.f,
		float PlayRate = 0.f,
		bool bLoopping = false,
		bool bReversePlayback = false,
		FVector Scale = FVector::OneVector);

	UPROPERTY()
	ADirectionalLight* pDirectionalLight;

	UPROPERTY()
	UMeshComponent* PossessedShadowComponent;
};