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
 * The class responsible for world generation. At the moment it must be placed in the world manually..
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMWorldGenerator : public AActor
{
	GENERATED_UCLASS_BODY()

public:

	/** Loads or generates the blocks within Active Zone radius. */
	void InitSurroundingArea(); //TODO: Maybe rename to LoadOrGenerateArea and adapt for teleport usage as well

	/** Deletes all static and optionally dynamic actors. If the BlockMetadata didn't exist, create it. */
	UBlockMetadata* EmptyBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects);

	/** First try to look at save, generate new if not found */
	void LoadOrGenerateBlock(const FIntPoint& BlockIndex, bool bRegenerationFeature = true);
	
	/** Generate a block */
	void RegenerateBlock(const FIntPoint& BlockIndex, bool KeepDynamicObjects = true, bool IgnoreConstancy = false);

	/** Turns on all actors in the active zone, turn off all others*/
	void UpdateActiveZone(const FIntPoint& CenterBlock);

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

	UMMetadataManager* GetMetadataManager() const { return MetadataManager; }

	UMDropManager* GetDropManager() const { return DropManager; }

	UMSaveManager* GetSaveManager() const { return SaveManager; }

	UFUNCTION(BlueprintCallable)
	UMReputationManager* GetReputationManager() const { return ReputationManager; }

	UFUNCTION(BlueprintCallable)
	UMExperienceManager* GetExperienceManager() const { return ExperienceManager; }

	UFUNCTION(BlueprintCallable)
	AMCommunicationManager* GetCommunicationManager() const { return CommunicationManager; }

	UMBlockGenerator* GetBlockGenerator() const { return BlockGenerator; }

	UMRoadManager* GetRoadManager() const { return RoadManager; }

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
	void SetupInputComponent();

	// TODO: Remove excess meta modifiers
	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<UMDropManager> DropManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<UMReputationManager> ReputationManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator)
	TSubclassOf<UMExperienceManager> ExperienceManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator)
	TSubclassOf<UMRoadManager> RoadManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator)
	TSubclassOf<UMSaveManager> SaveManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<AMCommunicationManager> CommunicationManagerBPClass;

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
	void OnPlayerChangedBlock(const FIntPoint& IN_OldBlockIndex, const FIntPoint& IN_NewBlockIndex);

	void GenerateNewPieceOfPerimeter(const FIntPoint& CenterBlock);

	void SetBiomesForBlocks(const FIntPoint& CenterBlock, TSet<FIntPoint>& BlocksToGenerate);

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
	TSet<FIntPoint> PendingBlocks;

private: // Saved to memory

	/** The number of blocks player passed since the last biomes perimeter coloring */
	int BlocksPassedSinceLastPerimeterColoring;

private:

	/** Matches actor names/Uids/etc. with their metadata.
	 * Once a world is loaded, ActorMetadata is available only for actors from visited blocks. */
	UPROPERTY()
	UMMetadataManager* MetadataManager;

	UPROPERTY()
	TSet<FIntPoint> ActiveBlocksMap;

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
