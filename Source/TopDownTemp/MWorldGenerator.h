// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MWorldGeneratorTypes.h"
#include "MWorldGenerator.generated.h"

class UMDropManager;
class AMGroundBlock;
class AMTree;
class AMActor;
class AMCharacter;
class AMMemoryator;

/**
 * The class responsible for world generation. At the moment it must be placed in the world manually..
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMWorldGenerator : public AActor
{
	GENERATED_UCLASS_BODY()

public:

	/** One-time generation. Performed at the game start to create the surrounding area */
	void GenerateActiveZone();

	void GenerateBlock(const FIntPoint& BlockIndex, bool EraseDynamicObjects = false);

	/** Turns on all actors in the active zone, turn off all others*/
	void UpdateActiveZone();

	template< class T >
	T* SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation, FActorSpawnParameters& SpawnParameters, bool bForceAboveGround = false)
	{
		if (SpawnParameters.Name.IsNone() && GetWorld())
		{
			SpawnParameters.Name = MakeUniqueObjectName(GetWorld(), Class);
		}
		return CastChecked<T>(SpawnActor(Class, Location, Rotation, SpawnParameters, bForceAboveGround),ECastCheckedType::NullAllowed);
	}

	void EnrollActorToGrid(AActor* Actor, bool bMakeBlockConstant = false);

	TSubclassOf<AActor> GetClassToSpawn(FName Name); 

	TMap<FName, AActor*> GetActorsInRect(FVector UpperLeft, FVector BottomRight, bool bDynamic);

	void CleanArea(const FVector& Location, float Radius);

	static FBoxSphereBounds GetDefaultBounds(UClass* IN_ActorClass, UObject* WorldContextObject);

	template< class T >
	T* SpawnActorInRadius(TSubclassOf<AActor> Class, float ToSpawnRadius = 150.f, float ToSpawnHeight = 0.f)
	{
		return CastChecked<T>(SpawnActorInRadius(Class, ToSpawnRadius, ToSpawnHeight),ECastCheckedType::NullAllowed);
	}

	UMDropManager* GetDropManager() const { return DropManager; }

	const TMap<FIntPoint, UBlockOfActors*>& GetGridOfActors() { return GridOfActors; }

	FIntPoint GetGroundBlockIndex(FVector Position);

	FVector GetGroundBlockLocation(FIntPoint BlockIndex);

protected:

#if WITH_EDITOR
	virtual ~AMWorldGenerator() override { DefaultBoundsMap.Empty(); };
#endif

	UPROPERTY(EditDefaultsOnly, Category=AMWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<UMDropManager> DropManagerBPClass;

private:

	AActor* SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters, bool bForceAboveGround);

	AActor* SpawnActorInRadius(UClass* Class, float ToSpawnRadius, const float ToSpawnHeight);

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	/** Matches all enabled dynamic actors with the blocks they are on*/
	void CheckDynamicActorsBlocks();

	/** Moves the navigation mesh to the player's position */
	void UpdateNavigationMesh();

	UFUNCTION()
	void OnPlayerChangedBlock(const FIntPoint& NewBlock);

	void SetBiomesForBlocks(const FIntPoint& CenterBlock, TSet<FIntPoint>& BlocksToGenerate);

	/** Function for spreading heavy GenerateBlock calls over multiple ticks */
	void OnTickGenerateBlocks(TSet<FIntPoint> BlocksToGenerate);

	static FVector RaycastScreenPoint(const UObject* pWorldContextObject, const EScreenPoint ScreenPoint);

	/** Lists all the blocks lying on the perimeter of the circle with the given coordinates and radius */ //TODO: Use Bresenham's Circle Algorithm for better performance
	static TSet<FIntPoint> GetBlocksOnPerimeter(int BlockX, int BlockY, int RadiusInBlocks);

	/** Lists all the blocks lying within the circle with the given coordinates and radius */
	TSet<FIntPoint> GetBlocksInRadius(int BlockX, int BlockY, int RadiusInBlocks) const;

	/** The radius of the visible area (in blocks) */
	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	int ActiveZoneRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SubclassessToSpawn, meta = (DisplayThumbnail, AllowPrivateAccess = true))
	TMap<FName, TSubclassOf<AActor>> ToSpawnActorClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SubclassessToSpawn, meta = (AllowPrivateAccess = "true"))
	TMap<FName, TSubclassOf<UObject>> ToSpawnComplexStructureClasses;

	/** The number of blocks to be changed before changing the perimeter coloring */
	UPROPERTY(EditDefaultsOnly, Category = AMWorldGenerator, meta = (AllowPrivateAccess = "true"))
	int BiomesPerimeterColoringRate = 10;

	/** The number of blocks player passed since the last biomes perimeter coloring */
	int BlocksPassedSinceLastPerimeterColoring;

	UPROPERTY()
	TMap<FIntPoint, UBlockOfActors*> GridOfActors;

	UPROPERTY()
	TMap<FName, FActorWorldMetadata> ActorsMetadata;

	TMap<FIntPoint, bool> ActiveBlocksMap;

	// TODO: Use FTimerHandle
	float DynamicActorsCheckInterval;
	float DynamicActorsCheckTimer;

	static TMap<UClass*, FBoxSphereBounds> DefaultBoundsMap;

	UPROPERTY()
	UMDropManager* DropManager;
};