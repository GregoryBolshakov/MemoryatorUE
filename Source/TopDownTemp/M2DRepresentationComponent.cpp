#include "M2DRepresentationComponent.h"
#include "Components/CapsuleComponent.h"
#include "PaperSpriteComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MRotatableFlipbookComponent.h"

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

	SetUpSprites();

	CameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
}

void UM2DRepresentationComponent::SetUpSprites()
{
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

void UM2DRepresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FaceToCamera();
}

void UM2DRepresentationComponent::SetMeshByRotation(float Angle, const FName& Tag)
{
	for (const auto RenderComponent : RenderComponentArray)
	{
		if (const auto RotatableFlipbook = Cast<UMRotatableFlipbookComponent>(RenderComponent);
			Tag == "" || RotatableFlipbook->ComponentTags.Contains(FName(Tag)))
		{
			RotatableFlipbook->SetFlipbookByRotation(Angle);
		}
	}
}

float UM2DRepresentationComponent::GetCameraDeflectionAngle(const UObject* WorldContextObject, FVector GazeDirection)
{
	const UWorld* pWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!pWorld)
	{
		return 0.f;
	}

	auto CameraVector = pWorld->GetFirstPlayerController()->PlayerCameraManager->GetActorForwardVector();
	CameraVector.Z = 0;

	GazeDirection.Z = 0;

	const FVector CrossProduct = FVector::CrossProduct(CameraVector, GazeDirection);
	float Angle = FMath::RadiansToDegrees(FMath::Atan2(CrossProduct.Size(), FVector::DotProduct(CameraVector, GazeDirection)));
	const FVector UpVector(0.f, 0.f, 1.f);
	float Sign = FVector::DotProduct(UpVector, CrossProduct) < 0.f ? -1.f : 1.f;
	Angle *= Sign;

	return Angle;
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
