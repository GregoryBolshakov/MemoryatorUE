#pragma once

#include "CoreMinimal.h"

#include "MGroundMarker.generated.h"

class UMRoadManager;
class AMWorldGenerator;

/** Helper class for rendering all geometry information about ground blocks, chunks and regions for debugging purposes. */
UCLASS()
class UMGroundMarker : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AMWorldGenerator* WorldGenerator, UMRoadManager* RoadManager);

	void Render(float DeltaSeconds) const;

private:
	AMWorldGenerator* pWorldGenerator;
	UMRoadManager* pRoadManager;
};
