// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MWorldGeneratorTypes.h"
#include "MWorldGenerator.generated.h"

#define ECC_Pickable ECollisionChannel::ECC_GameTraceChannel2
#define ECC_OccludedTerrain ECollisionChannel::ECC_GameTraceChannel3

class UMMetadataManager;
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
 * The class responsible for world generation.
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMWorldGenerator : public AActor
{
	GENERATED_UCLASS_BODY()

public:

	/** Loads or generates the blocks within Active Zone radius around given block. */
	void InitSurroundingArea(const FIntPoint& PlayerBlock, const uint8 ObserverIndex);

	/** Deletes all static and optionally dynamic actors. If the BlockMetadata didn't exist, create it. */
	UBlockMetadata* EmptyBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects);

	/** Loads or generates a character for a player that just has logged in. Then init surrounding area */
	void ProcessConnectingPlayer(APlayerController* NewPlayer);

	/** First try to look at save, generate new if not found */
	void LoadOrGenerateBlock(const FIntPoint& BlockIndex, bool bRegenerationFeature, const uint8 ObserverIndex);
	
	/** Generate a block */
	void RegenerateBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects = true, bool IgnoreConstancy = false);

	/** Sets the observer's flag on all blocks within a zone at the given center and of radius = ActiveZoneRadius */
	void AddObserverToZone(const FIntPoint& CenterBlock, const uint8 ObserverIndex);

	/** Removes the observer's flag on all blocks within a zone at the given center and of radius = ActiveZoneRadius */
	void RemoveObserverFromZone(const FIntPoint& CenterBlock, const uint8 ObserverIndex);

	/** Finds the difference between the new and old zones, removes the observer flag on the abandoned old one and sets it on the entered new parts */
	void MoveObserverToZone(const FIntPoint& CenterBlockFrom, const FIntPoint& CenterBlockTo, const uint8 ObserverIndex);

	template< class T >
	T* SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters(), bool bForceAboveGround = false, const FOnSpawnActorStarted& OnSpawnActorStarted = {}, const FMUid& Uid = {})
	{
		return CastChecked<T>(SpawnActor(Class, Location, Rotation, SpawnParameters, bForceAboveGround, OnSpawnActorStarted, Uid),ECastCheckedType::NullAllowed);
	}

	//TODO:Move this to MetadataManager
	void EnrollActorToGrid(AActor* Actor, const FMUid& Uid = {});

	/** Matches all enabled dynamic actors with the blocks they are on. Triggers all OnBlockChangedDelegates*/
	void CheckDynamicActorsBlocks();

	TSubclassOf<AActor> GetClassToSpawn(FName Name); 

	TMap<FName, AActor*> GetActorsInRect(FVector UpperLeft, FVector BottomRight, bool bDynamic);

	//TODO: Check for correct block constancy handling
	/** Deletes all static actors(trees, stones, etc.) including the ground block for each block within radius.\n
	 * Keeps dynamic actors, ignores blocks constancy. If some block's BlockMetadata didn't exist, create it. */
	//void CleanArea(const FVector& Location, int RadiusInBlocks, UPCGGraph* OverridePCGGraph = nullptr);

	void RegenerateArea(const FVector& Location, int RadiusInBlocks, UPCGGraph* OverridePCGGraph = nullptr);

	static FBoxSphereBounds GetDefaultBounds(UClass* IN_ActorClass, UObject* WorldContextObject);

	template< class T >
	T* SpawnActorInRadius(TSubclassOf<AActor> Class, const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters, float ToSpawnRadius = 150.f, float ToSpawnHeight = 0.f, const FOnSpawnActorStarted& OnSpawnActorStarted = {})
	{
		return CastChecked<T>(SpawnActorInRadius(Class, Location, Rotation, SpawnParameters, ToSpawnRadius, ToSpawnHeight, OnSpawnActorStarted),ECastCheckedType::NullAllowed);
	}

	UMBlockGenerator* GetBlockGenerator() const { return BlockGenerator; }

	FVector GetGroundBlockSize() const; //TODO: Can be done static, since WorldGenerator is now statically accessed from GameMode

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

	void SetupInputComponent();

protected:

	// TODO: Remove excess meta modifier
	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<UMBlockGenerator> BlockGeneratorBPClass;

private:

	AActor* SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters, bool bForceAboveGround, const FOnSpawnActorStarted& OnSpawnActorStarted, const FMUid& Uid);

	AActor* SpawnActorInRadius(UClass* Class, const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters, float ToSpawnRadius, const float ToSpawnHeight, const FOnSpawnActorStarted& OnSpawnActorStarted);

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	/** Moves the navigation mesh to the player's position */
	void UpdateNavigationMesh();

	UFUNCTION()
	void OnPlayerChangedBlock(const FIntPoint& IN_OldBlockIndex, const FIntPoint& IN_NewBlockIndex, const uint8 ObserverIndex);

	void GenerateNewPieceOfPerimeter(const FIntPoint& CenterBlock, const uint8 ObserverIndex);

	void SetBiomesForBlocks(const FIntPoint& CenterBlock, TSet<FIntPoint>& BlocksToGenerate, const uint8 ObserverIndex);

	/** Function for spreading heavy GenerateBlock calls over multiple ticks */
	void OnTickGenerateBlocks();

	static FVector RaycastScreenPoint(const UObject* pWorldContextObject, const EScreenPoint ScreenPoint);

	void DrawDebuggingInfo() const;

	/** The radius of the visible area (in blocks) */
	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	int ActiveZoneRadius = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SubclassessToSpawn, meta = (DisplayThumbnail, AllowPrivateAccess = true))
	TMap<FName, TSubclassOf<AActor>> ToSpawnActorClasses;

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

	/** Pool of blocks waiting to be generated. */
	TSet<FBlockAndObserver> PendingBlocks;

private: // Saved to memory

	/** The number of blocks player passed since the last biomes perimeter coloring */
	int BlocksPassedSinceLastPerimeterColoring;

private:

	UPROPERTY()
	TSet<FIntPoint> ActiveBlocksMap;

	UPROPERTY()
	TMap<UClass*, FBoxSphereBounds> DefaultBoundsMap;

private:

	UPROPERTY()
	UMBlockGenerator* BlockGenerator = nullptr;
};
