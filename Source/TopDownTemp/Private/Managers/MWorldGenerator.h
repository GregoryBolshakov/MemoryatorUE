// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MWorldGeneratorTypes.h"
#include "MWorldGenerator.generated.h"

class AMPickableActor;
class UMExperienceManager;
class UMReputationManager;
class AMCommunicationManager;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpawnActorStarted, AActor*)

class UMBlockGenerator;
class UMDropManager;
class AMGroundBlock;
class AMTree;
class AMActor;
class AMCharacter;
class AMMemoryator;

inline FString SaveGameWorldSlot {"WorldSave"}; // Move this into USaveGameWorld
/**
 * The class responsible for world generation. At the moment it must be placed in the world manually..
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMWorldGenerator : public AActor
{
	GENERATED_UCLASS_BODY()

public:

	/** Performed at the game start to create the surrounding area */
	void PrepareVisibleZone();

	void GenerateBlock(const FIntPoint& BlockIndex, bool EraseDynamicObjects = false);

	/** Turns on all actors in the active zone, turn off all others*/
	void UpdateActiveZone();

	template< class T >
	T* SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation, const FActorSpawnParameters& SpawnParameters = {}, bool bForceAboveGround = false, const FOnSpawnActorStarted& OnSpawnActorStarted = {})
	{
		return CastChecked<T>(SpawnActor(Class, Location, Rotation, SpawnParameters, bForceAboveGround, OnSpawnActorStarted),ECastCheckedType::NullAllowed);
	}

	void EnrollActorToGrid(AActor* Actor, bool bMakeBlockConstant = false);

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

	UFUNCTION(BlueprintCallable)
	UMReputationManager* GetReputationManager() const { return ReputationManager; }

	UFUNCTION(BlueprintCallable)
	UMExperienceManager* GetExperienceManager() const { return ExperienceManager; }

	UFUNCTION(BlueprintCallable)
	AMCommunicationManager* GetCommunicationManager() const { return CommunicationManager; }

	const TMap<FIntPoint, UBlockOfActors*>& GetGridOfActors() { return GridOfActors; }

	FVector GetGroundBlockSize();

	FIntPoint GetGroundBlockIndex(FVector Position);

	FVector GetGroundBlockLocation(FIntPoint BlockIndex);

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

	void LoadFromMemory();

	void SetupAutoSaves();

	/** The radius of the visible area (in blocks) */
	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	int ActiveZoneRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SubclassessToSpawn, meta = (DisplayThumbnail, AllowPrivateAccess = true))
	TMap<FName, TSubclassOf<AActor>> ToSpawnActorClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SubclassessToSpawn, meta = (AllowPrivateAccess = "true"))
	TMap<FName, TSubclassOf<UObject>> ToSpawnComplexStructureClasses;

	/** The number of blocks to be changed before changing the perimeter coloring */
	UPROPERTY(EditDefaultsOnly, Category = MWorldGenerator, meta = (AllowPrivateAccess = "true"))
	int BiomesPerimeterColoringRate = 10;

	static TMap<UClass*, FBoxSphereBounds> DefaultBoundsMap;

private: // Saved to memory

	/** The number of blocks player passed since the last biomes perimeter coloring */
	int BlocksPassedSinceLastPerimeterColoring;

	UPROPERTY()
	TMap<FIntPoint, UBlockOfActors*> GridOfActors;

	UPROPERTY()
	TMap<FName, FActorWorldMetadata> ActorsMetadata;

	UPROPERTY()
	TMap<FIntPoint, bool> ActiveBlocksMap;

private: // Managers

	UPROPERTY()
	UMDropManager* DropManager;

	UPROPERTY()
	UMReputationManager* ReputationManager;

	UPROPERTY()
	UMExperienceManager* ExperienceManager;

	//TODO: Fix needed: items disappear when game crashes/closes during a trade after items were moved to the widget
	UPROPERTY()
	AMCommunicationManager* CommunicationManager;

	UPROPERTY()
	UMBlockGenerator* BlockGenerator;
};
