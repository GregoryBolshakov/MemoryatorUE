#pragma once

#include "CoreMinimal.h"
#include "MRoadManager.generated.h"

class AMWorldGenerator;

UCLASS(Blueprintable)
class TOPDOWNTEMP_API UMRoadManager : public UObject
{
	GENERATED_BODY()

public:
	void GenerateNewPieceForRoads(const TSet<FIntPoint>& BlocksOnPerimeter, const AMWorldGenerator* WorldGenerator); 
private:
	
};

