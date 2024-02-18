// Fill out your copyright notice in the Description page of Project Settings.

#include "MActor.h"

#include "Components/M2DRepresentationComponent.h"
#include "Components/MInventoryComponent.h"
#include "Components/MIsActiveCheckerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Managers/MMetadataManager.h"
#include "Managers/MWorldGenerator.h"
#include "Managers/MWorldManager.h"

AMActor::AMActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PointComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootPoint"));
	SetRootComponent(PointComponent);

	IsActiveCheckerComponent = CreateDefaultSubobject<UMIsActiveCheckerComponent>(TEXT("IsActiveChecker"));
	IsActiveCheckerComponent->OnDisabledDelegate.BindUObject(this, &AMActor::OnDisabled);
	IsActiveCheckerComponent->OnEnabledDelegate.BindUObject(this, &AMActor::OnEnabled);

	InventoryComponent = CreateDefaultSubobject<UMInventoryComponent>(TEXT("Inventory"));
}

bool AMActor::Destroy(bool bNetForce, bool bShouldModifyLevel)
{
	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				pWorldGenerator->GetMetadataManager()->Remove(FName(GetName()));
				return true;
			}
		}
	}
	return false;
}

void AMActor::InitialiseInventory(const TArray<FItem>& IN_Items) const
{
	InventoryComponent->Initialize(IN_Items.Num(), IN_Items);
}

void AMActor::BeginLoadFromSD(const FMActorSaveData& MActorSD)
{
}

void AMActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (IsRandomizedAppearance)
	{
		RandomizeAppearanceID();
	}

	ApplyAppearanceID();

	OptionalRepresentationComponent = Cast<UM2DRepresentationComponent>(GetComponentByClass(UM2DRepresentationComponent::StaticClass()));
	if (OptionalRepresentationComponent)
	{
		OptionalRepresentationComponent->PostInitChildren();
	}

	CreateDynamicMaterials();
}

void AMActor::CreateDynamicMaterials()
{
	TArray<UStaticMeshComponent*> StaticMeshComps;
	GetComponents<UStaticMeshComponent>(StaticMeshComps);
	for (const auto StaticMeshComp : StaticMeshComps)
	{
		auto& RecordPerMesh = DynamicMaterials.FindOrAdd(StaticMeshComp);
		const auto Materials = StaticMeshComp->GetMaterials();
		for (int i = 0; i < Materials.Num(); ++i)
		{
			const auto DynamicMaterial = UMaterialInstanceDynamic::Create(Materials[i], this);
			StaticMeshComp->SetMaterial(i, DynamicMaterial);
			RecordPerMesh.Add(DynamicMaterial);
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

				if (const auto MyBlockInGrid = pWorldGenerator->GetMetadataManager()->FindOrAddBlock(MyIndex))
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
