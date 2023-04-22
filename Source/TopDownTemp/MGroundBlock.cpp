// Fill out your copyright notice in the Description page of Project Settings.


#include "MGroundBlock.h"
#include "MWorldGenerator.h"
#include "MWorldManager.h"

#include "PaperSpriteComponent.h"

AMGroundBlock::AMGroundBlock(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	GroundSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("GroundSprite"));
	GroundSpriteComponent->SetupAttachment(RootComponent);

	GroundTransitionSpriteLeft = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("GroundTransitionSpriteLeft"));
	GroundTransitionSpriteLeft->SetupAttachment(GroundSpriteComponent);

	GroundTransitionSpriteRight = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("GroundTransitionSpriteRight"));
	GroundTransitionSpriteRight->SetupAttachment(GroundSpriteComponent);

	GroundTransitionSpriteTop = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("GroundTransitionSpriteTop"));
	GroundTransitionSpriteTop->SetupAttachment(GroundSpriteComponent);

	GroundTransitionSpriteBottom = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("GroundTransitionSpriteBottom"));
	GroundTransitionSpriteBottom->SetupAttachment(GroundSpriteComponent);
}

void AMGroundBlock::UpdateBiome(EBiome IN_Biome)
{
	// Get the grid of actors to access adjacent blocks (for their possible disabling)
	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				const auto GridOfActors = pWorldGenerator->GetGridOfActors();

				const FIntPoint MyIndex = pWorldGenerator->GetGroundBlockIndex(GetActorLocation());

				// Disable transitions if they contact with the same biome or the adjacent block doesn't even exist
				TArray<FIntPoint> AdjacentBlockOffsets{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}; // Left; Right; Top; Bottom
				for (const auto& AdjacentBlockOffset : AdjacentBlockOffsets)
				{
					const auto Block = GridOfActors.Find({ MyIndex.X + AdjacentBlockOffset.X, MyIndex.Y + AdjacentBlockOffset.Y });
					if (!Block || (*Block)->Biome == IN_Biome)
					{
						if (const auto Transition = GetTransitionByOffset(AdjacentBlockOffset))
							Transition->SetHiddenInGame(true);
					}
					else if ((*Block)->pGroundBlock)
					{
						(*Block)->pGroundBlock->UpdateTransition(AdjacentBlockOffset * -1, IN_Biome); // *-1 because we update the opposite side of the adjacent block
					}
				}
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

UPaperSpriteComponent* AMGroundBlock::GetTransitionByOffset(FIntPoint Offset) const
{
	if (Offset == FIntPoint(-1, 0))
	{
		return GroundTransitionSpriteLeft;
	}
	if (Offset == FIntPoint(1, 0))
	{
		return GroundTransitionSpriteRight;
	}
	if (Offset == FIntPoint(0, -1))
	{
		return GroundTransitionSpriteTop;
	}
	if (Offset == FIntPoint(0, 1))
	{
		return GroundTransitionSpriteBottom;
	}
	check(false);
	return nullptr;
}

EBiome AMGroundBlock::GetMyBiome()
{
	if (const auto pWorld = GetWorld())
	{
		if (const auto pWorldManager = pWorld->GetSubsystem<UMWorldManager>())
		{
			if (const auto pWorldGenerator = pWorldManager->GetWorldGenerator())
			{
				const auto GridOfActors = pWorldGenerator->GetGridOfActors();

				const FIntPoint MyIndex = pWorldGenerator->GetGroundBlockIndex(GetActorLocation());

				if (const auto MyBlockInGrid = GridOfActors.Find(MyIndex))
				{
					return (*MyBlockInGrid)->Biome;
				}
			}
		}
	}
	check(false);
	return EBiome::DarkWoods;
}
