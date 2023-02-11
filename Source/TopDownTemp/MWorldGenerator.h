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

	AActor* SpawnActor(UClass* Class, FVector const& Location, FRotator const& Rotation, const FActorSpawnParameters& SpawnParameters);

	//TODO: Add another templated SpawnActor with SpawnParameters argument
	template< class T >
	T* SpawnActor(UClass* Class, FVector const& Location, FRotator const& Rotation, FName const& Name = "")
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = Name != "" ? Name : FName(FString::FromInt(++UniqueNameSuffix));
		return CastChecked<T>(SpawnActor(Class, Location, Rotation, SpawnParameters),ECastCheckedType::NullAllowed);
	}

	TSubclassOf<AActor> GetClassToSpawn(FName Name); 

	TMap<FName, FActorWorldMetadata> GetActorsInRect(FVector UpperLeft, FVector BottomRight, bool bDynamic);

private:

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
	TMap<FName, TSubclassOf<AActor>> ToSpawnActorMap;

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