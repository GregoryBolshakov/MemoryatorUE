#include "MRoadManager.h"
#include "MWorldGenerator.h"
#include "Kismet/GameplayStatics.h"

void UMRoadManager::GenerateNewPieceForRoads(const TSet<FIntPoint>& BlocksOnPerimeter, const AMWorldGenerator* WorldGenerator)
{
	const auto World = GetWorld();
	if (!IsValid(World)) return;
	const auto Player = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!IsValid(Player)) return;

	const auto PlayerLocation = Player->GetTransform().GetLocation();
	const auto PlayerBlock = WorldGenerator->GetGroundBlockIndex(PlayerLocation);

	const auto BlocksInRadius = WorldGenerator->GetBlocksInRadius(PlayerBlock.X, PlayerBlock.Y, WorldGenerator->GetActiveZoneRadius());

	for (const auto& PerimeterBlockIndex : BlocksOnPerimeter)
	{
		for (int XOffset = -1; XOffset <= 1; ++XOffset)
		{
			for (int YOffset = -1; YOffset <= 1; ++YOffset)
			{
				if (BlocksInRadius.Contains({PerimeterBlockIndex.X + XOffset, PerimeterBlockIndex.Y + YOffset}))
				{
					
				}
			}
		}
	}
}
