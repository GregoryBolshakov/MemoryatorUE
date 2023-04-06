// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MWorldGenerator.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBlockChanged);

class AMGroundBlock;
class AMTree;
class AMActor;
class AMCharacter;
class AMMemoryator;

/** Class for storing actors within one block of the frid */
USTRUCT()
struct FBlockOfActors
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TMap<FName, AActor*> StaticActors;

	UPROPERTY()
	TMap<FName, AActor*> DynamicActors;
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

/**
 * The class responsible for world generation. At the moment it must be placed in the world manually..
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMWorldGenerator : public AActor
{
	GENERATED_UCLASS_BODY()
public:

	void GenerateWorld();

	/** Turns on all actors in the active zone, turn off all others*/
	void UpdateActiveZone();

	template< class T >
	T* SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation, FActorSpawnParameters& SpawnParameters, bool bForceAboveGround = false)
	{
		if (SpawnParameters.Name.IsNone())
		{
			SpawnParameters.Name = MakeUniqueObjectName(this, Class);
		}
		return CastChecked<T>(SpawnActor(Class, Location, Rotation, SpawnParameters, bForceAboveGround),ECastCheckedType::NullAllowed);
	}

	TSubclassOf<AActor> GetClassToSpawn(FName Name); 

	TMap<FName, AActor*> GetActorsInRect(FVector UpperLeft, FVector BottomRight, bool bDynamic);

	void CleanArea(const FVector& Location, float Radius);

	static FBoxSphereBounds GetDefaultBounds(UClass* InActorClass);

	template< class T >
	T* SpawnActorInRadius(TSubclassOf<AActor> Class, float ToSpawnRadius = 150.f, float ToSpawnHeight = 0.f)
	{
		return CastChecked<T>(SpawnActorInRadius(Class, ToSpawnRadius, ToSpawnHeight),ECastCheckedType::NullAllowed);
	}

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
	void OnPlayerChangedBlock();

	FIntPoint GetGroundBlockIndex(FVector Position) const;

	UPROPERTY(EditAnywhere)
	FVector2D WorldSize{10000, 10000};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SubclassessToSpawn, meta = (DisplayThumbnail, AllowPrivateAccess = true))
	TMap<FName, TSubclassOf<AActor>> ToSpawnActorClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SubclassessToSpawn, meta = (AllowPrivateAccess = "true"))
	TMap<FName, TSubclassOf<UObject>> ToSpawnComplexStructureClasses;

	/** The box indicating the bounds of the interaction area of the world. I.e. rendering and ticking. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger Box", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* PlayerActiveZone;

	UPROPERTY()
	TMap<FIntPoint, FBlockOfActors> GridOfActors;

	UPROPERTY()
	TMap<FName, FActorWorldMetadata> ActorsMetadata;

	TMap<FIntPoint, bool> ActiveBlocksMap;

	float DynamicActorsCheckInterval;
	float DynamicActorsCheckTimer;

	int UniqueNameSuffix;
};