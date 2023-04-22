#pragma once

#include "CoreMinimal.h"
#include "MWorldGeneratorTypes.generated.h"

class AMGroundBlock;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBlockChanged, const FIntPoint&, NewBlock);

UENUM(BlueprintType)
enum class EBiome : uint8
{
	DarkWoods = 0,
	BirchGrove,
	Swamp
};

/** Used when generating random biomes coloring for generation perimeter */
USTRUCT()
struct FBiomeDelimiter
{
	GENERATED_BODY()
	int BlockPosition;
	EBiome Biome;
};

/** Class for storing actors within one block of the frid */
UCLASS()
class UBlockOfActors : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TMap<FName, AActor*> StaticActors;

	UPROPERTY()
	TMap<FName, AActor*> DynamicActors;

	bool IsConstant = false;

	EBiome Biome;

	//TODO: Come up how not to store it here. Temp workaround for ground block transitions
	AMGroundBlock* pGroundBlock = nullptr;
};

/** Utility class for storing actor's metadata in the grid */
USTRUCT()
struct FActorWorldMetadata
{
	GENERATED_BODY()
public:
	UPROPERTY()
	AActor* Actor;

	FIntPoint GroundBlockIndex;

	FOnBlockChanged OnBlockChangedDelegate;
};

enum class EScreenPoint
{
	TopLeft = 0,
	TopRight
};