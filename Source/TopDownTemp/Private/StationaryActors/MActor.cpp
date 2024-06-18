// Fill out your copyright notice in the Description page of Project Settings.

#include "MActor.h"

#include "Components/M2DRepresentationComponent.h"
#include "Components/MInventoryComponent.h"
#include "Components/MIsActiveCheckerComponent.h"
#include "Framework/MGameMode.h"
#include "Managers/MMetadataManager.h"
#include "Managers/MWorldGenerator.h"

AMActor::AMActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PointComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootPoint"));
	SetRootComponent(PointComponent);

	IsActiveCheckerComponent = CreateDefaultSubobject<UMIsActiveCheckerComponent>(TEXT("IsActiveChecker"));
	IsActiveCheckerComponent->OnDisabledDelegate.BindUObject(this, &AMActor::OnDisabled);
	IsActiveCheckerComponent->OnEnabledDelegate.BindUObject(this, &AMActor::OnEnabled);

	FaceCameraComponent = CreateDefaultSubobject<UM2DRepresentationComponent>(TEXT("FaceCameraComponent"));
	FaceCameraComponent->SetupAttachment(RootComponent);

	InventoryComponent = CreateDefaultSubobject<UMInventoryComponent>(TEXT("Inventory"));
}

bool AMActor::Destroy(bool bNetForce, bool bShouldModifyLevel)
{
	if (const auto MetadataManager = AMGameMode::GetMetadataManager(this))
	{
		MetadataManager->Remove(FName(GetName()));
		return true;
	}
	return false;
}

void AMActor::InitialiseInventory(const TArray<FItem>& IN_Items) const
{
	InventoryComponent->Initialize(IN_Items.Num(), IN_Items);
}

FMActorSaveData AMActor::GetSaveData() const
{
	FActorSaveData ActorSaveData = {
		GetClass(),
		GetActorLocation(),
		GetActorRotation(),
		AMGameMode::GetMetadataManager(this)->Find(FName(GetName()))->Uid,
		UMSaveManager::GetSaveDataForComponents(this) // TODO: Refactor this
	};

	FMActorSaveData MActorSD{
		ActorSaveData,
		GetAppearanceID(),
		GetIsRandomizedAppearance()
	};
	// Save inventory if the AMActor has it
	if (InventoryComponent)
	{
		MActorSD.InventoryContents = InventoryComponent->GetItemCopies(false);
	}

	return MActorSD;
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

	if (FaceCameraComponent)
	{
		FaceCameraComponent->PostInitChildren();
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
	if (const auto WorldGenerator = AMGameMode::GetWorldGenerator(this))
	{
		const FIntPoint MyIndex = WorldGenerator->GetGroundBlockIndex(GetActorLocation());

		if (const auto MyBlockInGrid = AMGameMode::GetMetadataManager(this)->FindOrAddBlock(MyIndex))
		{
			return MyBlockInGrid->Biome;
		}
	}
#ifndef WITH_EDITOR
	check(false);
#endif
	return EBiome::DarkWoods;
}
