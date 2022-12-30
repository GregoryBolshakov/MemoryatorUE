#include "M2DRepresentationComponent.h"
#include "Components/CapsuleComponent.h"
#include "PaperSpriteComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"

UM2DRepresentationComponent::UM2DRepresentationComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	CameraManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UM2DRepresentationComponent::BeginPlay()
{
	Super::BeginPlay();

	TArray<USceneComponent*> ChildComponents;
	GetChildrenComponents(false, ChildComponents);

	for (const auto& ChildComponent : ChildComponents)
	{
		if (ChildComponent->GetClass() == UPaperSpriteComponent::StaticClass())
		{
			RenderComponentArray.Add(dynamic_cast<UPaperSpriteComponent*>(ChildComponent));
		}
		if (ChildComponent->GetClass() == UCapsuleComponent::StaticClass())
		{
			CapsuleComponentArray.Add(dynamic_cast<UCapsuleComponent*>(ChildComponent));
		}
	}
	
	CameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
}

void UM2DRepresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FaceToCamera();
}

void UM2DRepresentationComponent::FaceToCamera()
{
	const auto CameraLocation = CameraManager->GetCameraLocation();
	const auto PlayerLocation = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetTransform().GetLocation();
	auto FarCameraLocation = CameraLocation + (CameraLocation - PlayerLocation);
	FarCameraLocation.Z /= 2; //TODO: bring the option out to the editor 

	for (const auto& RenderComponent : RenderComponentArray)
	{
		if (!IsValid(RenderComponent))
			continue;
		
		TArray<USceneComponent*> RenderComponentChildren;
		RenderComponent->GetChildrenComponents(false, RenderComponentChildren);
		
		for (const auto& Child : RenderComponentChildren)
		{
			if (Child->GetClass() == USceneComponent::StaticClass()) // The Child is the OriginPoint component
			{
				auto ChildLocation = Child->GetComponentTransform().GetLocation();
				const auto DirectionVector = ChildLocation - FarCameraLocation;
				
				const FRotator Rotation = FRotationMatrix::MakeFromX(DirectionVector).Rotator();
				RenderComponent->SetWorldRotation(Rotation);
				RenderComponent->AddLocalRotation(FRotator(0.f, 90.f, 0.f));

				// Move sprite so that its origin point matches its starting position.
				// (the point moved after sprite rotation, because it is attached to it) 
				RenderComponent->MoveComponent(ChildLocation - Child->GetComponentTransform().GetLocation(), RenderComponent->GetComponentQuat(), false);
				break;
			}
		}
	}
}
