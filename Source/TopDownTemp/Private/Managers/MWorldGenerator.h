// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MWorldGeneratorTypes.h"
#include "MWorldGenerator.generated.h"

#define ECC_Pickable ECollisionChannel::ECC_GameTraceChannel2
#define ECC_OccludedTerrain ECollisionChannel::ECC_GameTraceChannel3

class UMRoadManager;
class AMPickableActor;
class UMExperienceManager;
class UMReputationManager;
class UMDropManager;
class UMSaveManager;
class AMCommunicationManager;

class UMBlockGenerator;
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

	/** Executed once on first run to create the surrounding area */
	void InitNewWorld();

	UBlockMetadata* EmptyBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects, bool IgnoreConstancy = false);

	UBlockMetadata* GenerateBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects = true);

	/** Turns on all actors in the active zone, turn off all others*/
	void UpdateActiveZone();

	template< class T >
	T* SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters(), bool bForceAboveGround = false, const FOnSpawnActorStarted& OnSpawnActorStarted = {})
	{
		return CastChecked<T>(SpawnActor(Class, Location, Rotation, SpawnParameters, bForceAboveGround, OnSpawnActorStarted),ECastCheckedType::NullAllowed);
	}

	void EnrollActorToGrid(AActor* Actor);
	void RemoveActorFromGrid(AActor* Actor);

	TSubclassOf<AActor> GetClassToSpawn(FName Name); 

	TMap<FName, AActor*> GetActorsInRect(FVector UpperLeft, FVector BottomRight, bool bDynamic);

	void CleanArea(const FVector& Location, float Radius);

	static FBoxSphereBounds GetDefaultBounds(UClass* IN_ActorClass, UObject* WorldContextObject);

	template< class T >
	T* SpawnActorInRadius(TSubclassOf<AActor> Class, const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters, float ToSpawnRadius = 150.f, float ToSpawnHeight = 0.f, const FOnSpawnActorStarted& OnSpawnActorStarted = {})
	{
		return CastChecked<T>(SpawnActorInRadius(Class, Location, Rotation, SpawnParameters, ToSpawnRadius, ToSpawnHeight, OnSpawnActorStarted),ECastCheckedType::NullAllowed);
	}

	UMDropManager* GetDropManager() const { return DropManager; }

	UMSaveManager* GetSaveManager() const { return SaveManager; }

	UFUNCTION(BlueprintCallable)
	UMReputationManager* GetReputationManager() const { return ReputationManager; }

	UFUNCTION(BlueprintCallable)
	UMExperienceManager* GetExperienceManager() const { return ExperienceManager; }

	UFUNCTION(BlueprintCallable)
	AMCommunicationManager* GetCommunicationManager() const { return CommunicationManager; }

	UMBlockGenerator* GetBlockGenerator() const { return BlockGenerator; }

	UBlockMetadata* FindOrAddBlock(FIntPoint Index);

	FVector GetGroundBlockSize() const;

	FIntPoint GetGroundBlockIndex(FVector Position) const;

	/** Is more reliable than manually using player via UGameplayStatics,
	 * because player controller might be invalid during game shutdown */
	FIntPoint GetPlayerGroundBlockIndex() const;

	FVector GetGroundBlockLocation(FIntPoint BlockIndex);

	/** Lists all the blocks lying on the perimeter of the circle with the given coordinates and radius */ //TODO: Use Bresenham's Circle Algorithm for better performance
	static TSet<FIntPoint> GetBlocksOnPerimeter(int BlockX, int BlockY, int RadiusInBlocks);

	/** Lists all the blocks lying within the circle with the given coordinates and radius */
	static TSet<FIntPoint> GetBlocksInRadius(int BlockX, int BlockY, int RadiusInBlocks);

	int GetActiveZoneRadius() const { return ActiveZoneRadius; }

	TSubclassOf<AActor> GetActorClassToSpawn(FName Name);

protected:

#if WITH_EDITOR
	virtual ~AMWorldGenerator() override { DefaultBoundsMap.Empty(); };
#endif

	// TODO: Remove excess meta modifiers
	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<UMDropManager> DropManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<UMReputationManager> ReputationManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator)
	TSubclassOf<UMExperienceManager> ExperienceManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator)
	TSubclassOf<UMSaveManager> SaveManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<AMCommunicationManager> CommunicationManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<UMBlockGenerator> BlockGeneratorBPClass;

private:

	AActor* SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters, bool bForceAboveGround, const FOnSpawnActorStarted& OnSpawnActorStarted);

	AActor* SpawnActorInRadius(UClass* Class, const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters, float ToSpawnRadius, const float ToSpawnHeight, const FOnSpawnActorStarted& OnSpawnActorStarted);

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	/** Matches all enabled dynamic actors with the blocks they are on. Triggers all OnBlockChangedDelegates*/
	void CheckDynamicActorsBlocks();

	/** Moves the navigation mesh to the player's position */
	void UpdateNavigationMesh();

	UFUNCTION()
	void OnPlayerChangedBlock(const FIntPoint& IN_OldBlockIndex, const FIntPoint& IN_NewBlockIndex);

	void GenerateNewPieceOfPerimeter(const FIntPoint& CenterBlock);

	void SetBiomesForBlocks(const FIntPoint& CenterBlock, TSet<FIntPoint>& BlocksToGenerate);

	/** Function for spreading heavy GenerateBlock calls over multiple ticks */
	void OnTickGenerateBlocks(TSet<FIntPoint> BlocksToGenerate);

	static FVector RaycastScreenPoint(const UObject* pWorldContextObject, const EScreenPoint ScreenPoint);

	/** The radius of the visible area (in blocks) */
	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	int ActiveZoneRadius = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SubclassessToSpawn, meta = (DisplayThumbnail, AllowPrivateAccess = true))
	TMap<FName, TSubclassOf<AActor>> ToSpawnActorClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SubclassessToSpawn, meta = (AllowPrivateAccess = "true"))
	TMap<FName, TSubclassOf<UObject>> ToSpawnComplexStructureClasses;

	UPROPERTY(EditDefaultsOnly, Category = MWorldGenerator)
	EBiome BiomeForInitialGeneration;

	/** The number of blocks to be changed before changing the perimeter coloring */
	UPROPERTY(EditDefaultsOnly, Category = MWorldGenerator, meta = (AllowPrivateAccess = "true"))
	int BiomesPerimeterColoringRate = 10;

	/** If player changes block and it is not adjacent (due to lag/low fps/very high player speed)
	// we recreate the continuous path travelled to generate perimeter for each travelled block. Store those blocks here */
	TArray<FIntPoint> TravelledDequeue;

	/** Turns on after the player was teleported, is turned off by AMWorldGenerator::OnPlayerChangedBlock */
	bool bPendingTeleport = false;

private: // Saved to memory

	/** The number of blocks player passed since the last biomes perimeter coloring */
	int BlocksPassedSinceLastPerimeterColoring;

	UPROPERTY()
	TMap<FIntPoint, UBlockMetadata*> GridOfActors;

private:

	/** Maps all the actors in the world with their names.
	 * Once a world is loaded, ActorsMetadata is not immediately available. It loads in parallel */
	UPROPERTY()
	TMap<FName, FActorWorldMetadata> ActorsMetadata;

	UPROPERTY()
	TMap<FIntPoint, bool> ActiveBlocksMap;

	UPROPERTY()
	TMap<UClass*, FBoxSphereBounds> DefaultBoundsMap;

private: // Managers

	UPROPERTY()
	UMDropManager* DropManager = nullptr;

	UPROPERTY()
	UMReputationManager* ReputationManager = nullptr;

	UPROPERTY()
	UMExperienceManager* ExperienceManager = nullptr;

	UPROPERTY()
	UMSaveManager* SaveManager = nullptr;

	//TODO: Fix needed: items disappear when game crashes/closes during a trade after items were moved to the widget
	UPROPERTY()
	AMCommunicationManager* CommunicationManager = nullptr;

	UPROPERTY()
	UMRoadManager* RoadManager = nullptr;

	UPROPERTY()
	UMBlockGenerator* BlockGenerator = nullptr;
};
