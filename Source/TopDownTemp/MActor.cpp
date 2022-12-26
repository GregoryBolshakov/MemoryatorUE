// Fill out your copyright notice in the Description page of Project Settings.

#include "MActor.h"

#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/CollisionProfile.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Kismet/GameplayStatics.h"

AMActor::AMActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	OriginPointComponent = CreateDefaultSubobject<USceneComponent>(TEXT("OriginPoint"));
	//RootComponent = OriginPointComponent;
	
	RenderComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("RenderComponent"));
	RenderComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RenderComponent->Mobility = EComponentMobility::Movable;
	RenderComponent->AttachToComponent(OriginPointComponent, FAttachmentTransformRules::KeepRelativeTransform, TEXT("RenderSocket"));
	
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->AttachToComponent(OriginPointComponent, FAttachmentTransformRules::KeepRelativeTransform, TEXT("RenderSocket"));

	SetRootComponent(OriginPointComponent);
	
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void AMActor::BeginPlay()
{
	Super::BeginPlay();
	
	CameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
}

void AMActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	FaceToCamera();
}

void AMActor::FaceToCamera()
{
	const auto CameraLocation = CameraManager->GetCameraLocation();
	const auto PlayerLocation = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetTransform().GetLocation();
	auto FarCameraLocation = CameraLocation + (CameraLocation - PlayerLocation);
	FarCameraLocation.Z = 0;
	
	const auto DirectionVector = (this->GetTransform().GetLocation() - FarCameraLocation);

	const FRotator Rotation = FRotationMatrix::MakeFromX(DirectionVector).Rotator();
	this->SetActorRotation(Rotation);
}

#if WITH_EDITOR
bool AMActor::GetReferencedContentObjects(TArray<UObject*>& Objects) const
{
	Super::GetReferencedContentObjects(Objects);

	if (UPaperSprite* SourceSprite = RenderComponent->GetSprite())
	{
		Objects.Add(SourceSprite);
	}
	return true;
}
#endif
