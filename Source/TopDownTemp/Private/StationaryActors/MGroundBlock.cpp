// Fill out your copyright notice in the Description page of Project Settings.


#include "MGroundBlock.h"

#include "PaperSpriteComponent.h"
#include "Framework/MGameMode.h"
#include "Managers/MMetadataManager.h"
#include "Managers/MWorldGenerator.h"

AMGroundBlock::AMGroundBlock(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	GroundMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMesh"));
	GroundMeshComponent->SetupAttachment(RootComponent);

	GroundTransitionMeshLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundTransitionMeshLeft"));
	GroundTransitionMeshLeft->SetupAttachment(GroundMeshComponent);

	GroundTransitionMeshRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundTransitionMeshRight"));
	GroundTransitionMeshRight->SetupAttachment(GroundMeshComponent);

	GroundTransitionMeshTop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundTransitionMeshTop"));
	GroundTransitionMeshTop->SetupAttachment(GroundMeshComponent);

	GroundTransitionMeshBottom = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundTransitionMeshBottom"));
	GroundTransitionMeshBottom->SetupAttachment(GroundMeshComponent);
}

void AMGroundBlock::UpdateBiome(EBiome IN_Biome)
{
	// Get the grid of actors to access adjacent blocks (for possible disabling of their transitions)
	if (const auto WorldGenerator = AMGameMode::GetWorldGenerator(this))
	{
		const FIntPoint MyIndex = WorldGenerator->GetGroundBlockIndex(GetActorLocation());

		// Disable transitions if they contact with the same biome or the adjacent block doesn't even exist
		TArray<FIntPoint> AdjacentBlockOffsets{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}; // Left; Right; Top; Bottom
		for (const auto& AdjacentBlockOffset : AdjacentBlockOffsets)
		{
			const auto Block = AMGameMode::GetMetadataManager(this)->FindOrAddBlock({ MyIndex.X + AdjacentBlockOffset.X, MyIndex.Y + AdjacentBlockOffset.Y });
			if (!Block || Block->Biome == IN_Biome)
			{
				if (const auto Transition = GetTransitionByOffset(AdjacentBlockOffset))
					Transition->SetHiddenInGame(true);
			}
			else if (Block->pGroundBlock)
			{
				Block->pGroundBlock->UpdateTransition(AdjacentBlockOffset * -1, IN_Biome); // *-1 because we update the opposite side of the adjacent block
			}
		}
	}
	//TODO: Consider hiding lower block transitions

	OnBiomeUpdated();
}

void AMGroundBlock::UpdateTransition(FIntPoint Offset, EBiome AdjacentBiome)
{
	if (const auto Transition = GetTransitionByOffset(Offset))
	{
		Transition->SetHiddenInGame(GetMyBiome() == AdjacentBiome);
	}
}

UStaticMeshComponent* AMGroundBlock::GetTransitionByOffset(FIntPoint Offset) const
{
	if (Offset == FIntPoint(-1, 0))
	{
		return GroundTransitionMeshLeft;
	}
	if (Offset == FIntPoint(1, 0))
	{
		return GroundTransitionMeshRight;
	}
	if (Offset == FIntPoint(0, -1))
	{
		return GroundTransitionMeshTop;
	}
	if (Offset == FIntPoint(0, 1))
	{
		return GroundTransitionMeshBottom;
	}
	check(false);
	return nullptr;
}
