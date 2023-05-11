#include "M2DRepresentationComponent.h"

#include "M2DRepresentationBlueprintLibrary.h"
#include "M2DShadowControllerComponent.h"
#include "Components/CapsuleComponent.h"
#include "PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MRotatableFlipbookComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetMathLibrary.h"

UM2DRepresentationComponent::UM2DRepresentationComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	CameraManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UM2DRepresentationComponent::PostInitChildren()
{
	SetUpSprites();
	if (bFaceToCamera)
	{
		CreateShadowTwins();
	}
}

void UM2DRepresentationComponent::BeginPlay()
{
	Super::BeginPlay();

	CameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
}

void UM2DRepresentationComponent::SetUpSprites()
{
	CapsuleComponentArray.Empty();
	RenderComponentArray.Empty();
	TArray<USceneComponent*> ChildComponents;
	GetChildrenComponents(true, ChildComponents);

	for (const auto& ChildComponent : ChildComponents)
	{
		if (const auto RenderComponent = Cast<UMeshComponent>(ChildComponent))
		{
			RenderComponentArray.Add(RenderComponent);
		}
		if (const auto CapsuleComponent = Cast<UCapsuleComponent>(ChildComponent))
		{
			CapsuleComponentArray.Add(CapsuleComponent);
		}
	}
}

void UM2DRepresentationComponent::CreateShadowTwins()
{
	const auto pOwner = GetOwner();
	if (!pOwner || !bCastShadow)
	{
		return;
	}

	//TODO: Don't create another twins and controllers. We should search if there are already created in case of PostEditChangeProperty invalidation
	ShadowTwinComponentArray.Empty();
	ShadowTwinControllerArray.Empty();

	for (const auto& RenderComponent : RenderComponentArray)
	{
		UMeshComponent* ShadowComponent = nullptr;

		if (Cast<UWidgetComponent>(RenderComponent))
		{
			continue; // We don't need to cast shadows from widget meshes
		}
		if (const auto PaperSpriteComponent = Cast<UPaperSpriteComponent>(RenderComponent))
		{
			//TODO: In the future actors will use only UMRotatableFlipbookComponent, this case is temporary
			if (const auto TwinSpriteComponent = NewObject<UPaperSpriteComponent>(this, UPaperSpriteComponent::StaticClass(), *("ShadowTwin_" + RenderComponent->GetName()), RF_NoFlags, PaperSpriteComponent))
			{
				TwinSpriteComponent->SetSprite(PaperSpriteComponent->GetSprite());
				ShadowComponent = TwinSpriteComponent;
			}
		}
		else
		if (const auto FlipbookComponent = Cast<UMRotatableFlipbookComponent>(RenderComponent))
		{
			if (const auto TwinFlipbookComponent = NewObject<UMRotatableFlipbookComponent>(this, UMRotatableFlipbookComponent::StaticClass(), FName("ShadowTwin_" + RenderComponent->GetName()), RF_NoFlags, FlipbookComponent))
			{
				TwinFlipbookComponent->SetFlipbook(FlipbookComponent->GetFlipbook());
				ShadowComponent = TwinFlipbookComponent;
			}
		}

		if (ShadowComponent != nullptr)
		{
			// Creating and setting up the controller for the shadow component
			const auto ShadowControllerComponent = NewObject<UM2DShadowControllerComponent>(this, UM2DShadowControllerComponent::StaticClass(), FName(ShadowComponent->GetName() + "_Controller"));
			ShadowControllerComponent->RegisterComponent();
			ShadowControllerComponent->AttachToComponent(ShadowComponent, FAttachmentTransformRules::KeepRelativeTransform);
			ShadowControllerComponent->CreationMethod = EComponentCreationMethod::Instance;
			ShadowControllerComponent->Possess(ShadowComponent, RenderComponent);

			// Common object creation pipeline during runtime
			ShadowComponent->RegisterComponent();
			ShadowComponent->AttachToComponent(pOwner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			ShadowComponent->CreationMethod = EComponentCreationMethod::Instance;

			// Just copy the render component transform
			ShadowComponent->SetWorldTransform(RenderComponent->GetComponentTransform());

			ShadowTwinComponentArray.Add(ShadowComponent);
			ShadowTwinControllerArray.Add(ShadowControllerComponent);
		}
	}
}

void UM2DRepresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FaceToCamera();

	InterpolateColor(DeltaTime);
}

void UM2DRepresentationComponent::SetMeshByGazeAndVelocity(const FVector& IN_Gaze, const FVector& IN_Velocity,
	const FName& Tag)
{
	const auto Angle = UM2DRepresentationBlueprintLibrary::GetCameraDeflectionAngle(this, IN_Gaze);

	for (const auto RenderComponent : RenderComponentArray)
	{
		if (const auto RotatableFlipbook = Cast<UMRotatableFlipbookComponent>(RenderComponent);
			RotatableFlipbook && (Tag == "" || RotatableFlipbook->ComponentTags.Contains(FName(Tag))))
		{
			RotatableFlipbook->SetFlipbookByRotation(Angle);
		}
	}

	LastValidGaze = IN_Gaze;
}

void UM2DRepresentationComponent::SetColor(const FLinearColor& Color)
{
	DesiredColor = Color;
}

void UM2DRepresentationComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

#if WITH_EDITOR
	if (GIsPlayInEditorWorld)
	{
		// It is still poorly understood, but for some reason some of the stored pointers to components are invalidated
		// after changing parameters at runtime. Therefore, we point them again.
		PostInitChildren();
	}
#endif
}

void UM2DRepresentationComponent::InterpolateColor(const float DeltaTime)
{
	CurrentColor.R = UKismetMathLibrary::FInterpTo(CurrentColor.R, DesiredColor.R, DeltaTime, ColorChangingSpeed);
	CurrentColor.G = UKismetMathLibrary::FInterpTo(CurrentColor.G, DesiredColor.G, DeltaTime, ColorChangingSpeed);
	CurrentColor.B = UKismetMathLibrary::FInterpTo(CurrentColor.B, DesiredColor.B, DeltaTime, ColorChangingSpeed);
	CurrentColor.A = UKismetMathLibrary::FInterpTo(CurrentColor.A, DesiredColor.A, DeltaTime, ColorChangingSpeed);

	for (const auto RenderComponent : RenderComponentArray)
	{
		if (const auto Flipbook = Cast<UPaperFlipbookComponent>(RenderComponent))
		{
			Flipbook->SetSpriteColor(CurrentColor);
		}
		if (const auto Sprite = Cast<UPaperSpriteComponent>(RenderComponent))
		{
			Sprite->SetSpriteColor(CurrentColor);
		}
	}
}

void UM2DRepresentationComponent::FaceToCamera()
{
	if (!bFaceToCamera)
		return;

	if (!CameraManager)
	{
		check(false);
		return;
	}

	const auto CameraLocation = CameraManager->GetCameraLocation();
	//TODO: Not to get Pawn reference every tick; Remember this field and update by event (CPU)
	const auto PlayerLocation = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetTransform().GetLocation();
	auto FarCameraLocation = CameraLocation + (CameraLocation - PlayerLocation);
	FarCameraLocation.Z /= 2; //TODO: bring the option out to the editor

	for (const auto& RenderComponent : RenderComponentArray)
	{
		if (!IsValid(RenderComponent))
			continue;

		TArray<USceneComponent*> RenderComponentChildren;
		RenderComponent->GetChildrenComponents(false, RenderComponentChildren);

		// Try to find the first child with USceneComponent class. It will be the pivot point for the face-to-camera rotation
		const USceneComponent* OriginPoint = RenderComponent;
		for (const auto& Child : RenderComponentChildren)
		{
			if (Child->GetClass() == USceneComponent::StaticClass())
			{
				OriginPoint = Child;
				break;
			}
		}
		const auto OriginPointLocation = OriginPoint->GetComponentLocation();
		const auto DirectionVector = OriginPointLocation - FarCameraLocation;

		const FRotator Rotation = FRotationMatrix::MakeFromX(DirectionVector).Rotator();
		RenderComponent->SetWorldRotation(Rotation);
		if (!Cast<UWidgetComponent>(RenderComponent)) 
		{
			// we always add 90 for 2D objects because they are arranged along the x-axis, not across
			RenderComponent->AddLocalRotation(FRotator(0.f, 90.f, 0.f));
		}
		else
		{
			// For some reason widget components aren't arranged along the x-axis (as all paper sprites do).
			// Moreover, by default they look away from the camera, not at it
			RenderComponent->AddLocalRotation(FRotator(0.f, -180.f, 0.f));
		}
		RenderComponent->AddLocalRotation(RotationWhileFacingCamera);

		// Move sprite so that its origin point matches its starting position.
		// (the point might have moved after sprite rotation, because it is attached to it) 
		RenderComponent->MoveComponent(OriginPointLocation - OriginPoint->GetComponentLocation(), RenderComponent->GetComponentQuat(), false);
	}
}
