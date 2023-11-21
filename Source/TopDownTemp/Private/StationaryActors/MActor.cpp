// Fill out your copyright notice in the Description page of Project Settings.

#include "MActor.h"

#include "Components/M2DRepresentationComponent.h"
#include "Components/MIsActiveCheckerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Managers/MWorldGenerator.h"
#include "Managers/MWorldManager.h"

AMActor::AMActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PointComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootPoint"));
	SetRootComponent(PointComponent);

	RepresentationComponent = CreateDefaultSubobject<UM2DRepresentationComponent>(TEXT("Representation"));
	RepresentationComponent->SetupAttachment(RootComponent);

	IsActiveCheckerComponent = CreateDefaultSubobject<UMIsActiveCheckerComponent>(TEXT("IsActiveChecker"));
	IsActiveCheckerComponent->OnDisabledDelegate.BindUObject(this, &AMActor::OnDisabled);
	IsActiveCheckerComponent->OnEnabledDelegate.BindUObject(this, &AMActor::OnEnabled);
}

bool AMActor::Destroy(bool bNetForce, bool bShouldModifyLevel)
{
	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				pWorldGenerator->RemoveActorFromGrid(this);
				return true;
			}
		}
	}
	return false;
}

void AMActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (IsRandomizedAppearance)
	{
		RandomizeAppearanceID();
	}

	ApplyAppearanceID();

	RepresentationComponent->PostInitChildren();

	CreateDynamicMaterials();
}

void AMActor::CreateDynamicMaterials()
{
	TArray<UStaticMeshComponent*> StaticMeshComps;
	GetComponents<UStaticMeshComponent>(StaticMeshComps);
	for (const auto StaticMeshComp : StaticMeshComps)
	{
		const auto Materials = StaticMeshComp->GetMaterials();
		for (int i = 0; i < Materials.Num(); ++i)
		{
			const auto DynamicMaterial = UMaterialInstanceDynamic::Create(Materials[i], this);
			StaticMeshComp->SetMaterial(i, DynamicMaterial);
			DynamicMaterials.Add(StaticMeshComp, DynamicMaterial);
		}
	}
}

EBiome AMActor::GetMyBiome()
{
	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				const FIntPoint MyIndex = pWorldGenerator->GetGroundBlockIndex(GetActorLocation());

				if (const auto MyBlockInGrid = pWorldGenerator->GetBlock(MyIndex))
				{
					return MyBlockInGrid->Biome;
				}
			}
		}
	}
#ifndef WITH_EDITOR
	check(false);
#endif
	return EBiome::DarkWoods;
}
