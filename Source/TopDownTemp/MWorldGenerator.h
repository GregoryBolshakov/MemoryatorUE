// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MWorldGenerator.generated.h"

class AMGroundBlock;
class AMTree;

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

USTRUCT()
struct FActorWorldMetadata
{
	GENERATED_BODY()
public:
	UPROPERTY()
	AActor* Actor;

	FIntPoint GroundBlockIndex;
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

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	AActor* SpawnActor(UClass* Class, FVector const& Location, FRotator const& Rotation, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());

	//TODO: Add another templated SpawnActor with SpawnParameters argument
	template< class T >
	T* SpawnActor(UClass* Class, FVector const& Location, FRotator const& Rotation, FName const& Name)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = Name;
		return CastChecked<T>(SpawnActor(Class, Location, Rotation, SpawnParameters),ECastCheckedType::NullAllowed);
	}

	TMap<FName, FActorWorldMetadata> GetActorsInRect(FVector UpperLeft, FVector BottomRight, bool bDynamic);

	template<typename T>
	static FString GetStringByClass();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AMGroundBlock> ToSpawnGroundBlock;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AMActor> ToSpawnTree;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AMCharacter> ToSpawnNightmare;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AMMemoryator> ToSpawnMemoryator;

private:

	void UpdateActiveZone();

	FIntPoint GetGroundBlockIndex(FVector Position) const;

	UPROPERTY(EditAnywhere)
	FVector2D WorldSize{10000, 10000};
	
	/** The box indicating the bounds of the interaction area of the world. I.e. rendering and ticking. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger Box", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* PlayerActiveZone;

	UPROPERTY()
	TMap<FIntPoint, FBlockOfActors> GridOfActors;

	UPROPERTY()
	TMap<FName, FActorWorldMetadata> ActorsMetadata;

	TMap<FIntPoint, bool> ActiveBlocksMap;
};

template <typename T>
FString AMWorldGenerator::GetStringByClass()
{
	if (TIsSame<T, AMGroundBlock>::Value)
		return "GroundBlock";
	return "unknown";
}