#include "M2DShadowControllerComponent.h"

#include "ComponentUtils.h"
#include "PaperFlipbookComponent.h"
#include "Engine/DirectionalLight.h"
#include "Kismet/GameplayStatics.h"
#include "MRotatableFlipbookComponent.h"
#include "M2DRepresentationBlueprintLibrary.h"

UM2DShadowControllerComponent::UM2DShadowControllerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, pDirectionalLight(nullptr)
	, PossessedShadowComponent(nullptr)
{
	// We consider there is no light source in the blueprint editor
#ifdef WITH_EDITOR
	if (!GIsPlayInEditorWorld)
	{
		return;
	}
#endif

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UM2DShadowControllerComponent::Possess(UMeshComponent* ShadowComponentToPossess, UMeshComponent* RenderComponent)
{
	// We consider there is no light source in the blueprint editor
#ifdef WITH_EDITOR
	if (!GIsPlayInEditorWorld)
	{
		return;
	}
#endif

	PossessedShadowComponent = ShadowComponentToPossess;

	pDirectionalLight = Cast<ADirectionalLight>(UGameplayStatics::GetActorOfClass(this, ADirectionalLight::StaticClass()));
	check(pDirectionalLight);

	if (const auto FlipbookComponent = Cast<UMRotatableFlipbookComponent>(RenderComponent))
	{
		FlipbookComponent->OnFlipbookChangedDelegate.AddDynamic(this, &UM2DShadowControllerComponent::OnPossessedMeshUpdated);
	}

	// Configure shadows settings for the mesh in the world
	PossessedShadowComponent->CastShadow = true;
	PossessedShadowComponent->bCastDynamicShadow = true;
	PossessedShadowComponent->bCastHiddenShadow = true;
	PossessedShadowComponent->SetHiddenInGame(true);
	PossessedShadowComponent->SetVisibility(false);
}

void UM2DShadowControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Rotate the mesh perpendicularly to the direction light source
	if (PossessedShadowComponent && pDirectionalLight)
	{
		auto ShadowRotation = pDirectionalLight->GetTransform().Rotator();
		ShadowRotation.Pitch = 0.f;
		ShadowRotation.Yaw += 90.f;
		ShadowRotation.Roll = 0.f;
		PossessedShadowComponent->SetWorldRotation(ShadowRotation);
	}
}

void UM2DShadowControllerComponent::OnPossessedMeshUpdated(
	UPaperFlipbook* Flipbook,
	float PlaybackPosition,
	float PlayRate,
	bool bLoopping,
	bool bReversePlayback,
	FVector Scale)
{
	if (const auto PossessedFlipbookComponent = Cast<UPaperFlipbookComponent>(PossessedShadowComponent))
	{
		PossessedFlipbookComponent->SetFlipbook(Flipbook);

		if (!Flipbook)
		{
			return;
		}

		PossessedFlipbookComponent->SetPlaybackPosition(PlaybackPosition, false);
		PossessedFlipbookComponent->SetPlayRate(PlayRate);
		PossessedFlipbookComponent->SetLooping(bLoopping);
		if (bReversePlayback)
		{
			PossessedFlipbookComponent->Reverse();
		}

		// If the camera observes the mesh from the opposite side, it should be mirrored
		const auto angle = UM2DRepresentationBlueprintLibrary::GetCameraDeflectionAngle(this, pDirectionalLight->GetActorForwardVector());
		if (abs(angle) > 90.f)
		{
			Scale.X *= -1;
		}
		PossessedFlipbookComponent->SetRelativeScale3D(Scale);

		PossessedFlipbookComponent->Play();
	}
}
