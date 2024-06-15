#pragma once

#include "CoreMinimal.h"

#include "MGroundMarker.generated.h"

class UMRoadManager;
class AMWorldGenerator;

USTRUCT()
struct FReplicatedData
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FIntPoint> AdjacentRegions;
	UPROPERTY()
	FVector BlockSize;
	UPROPERTY()
	FIntPoint ChunkSizeInBlocks;
	UPROPERTY()
	FIntPoint RegionSizeInChunks;
	/** Need to replicate Enabled to know when to turn rendering off on clients */
	UPROPERTY()
	bool Enabled = false;
};

/** Helper class for rendering all geometry information about ground blocks, chunks and regions for debugging purposes.\n
 * Replicated. Sets replicated data to draw everything for the current frame in one call.
 * That's important for proper persistent lines flushing.\n\n
 * Architecture might change. This replication was needed to see geometry in detached F8 mode when playing as Listen Server.
 */
//TODO: Disable for Shipping builds
UCLASS()
class AMGroundMarker : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	void Initialize(AMWorldGenerator* WorldGenerator, UMRoadManager* RoadManager);

	/** Called from server only. It prepares all debug data we want to render */
	void SetReplicatedData();

	UFUNCTION()
	void RenderLocally();

	UFUNCTION(NetMulticast, Reliable)
	void OnToggleDebuggingGeometry();

protected:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	bool EnabledOnServer = false;

	UPROPERTY(ReplicatedUsing=RenderLocally)
	FReplicatedData ReplicatedData;

	UPROPERTY()
	AMWorldGenerator* pWorldGenerator;
	UPROPERTY()
	UMRoadManager* pRoadManager;
};
