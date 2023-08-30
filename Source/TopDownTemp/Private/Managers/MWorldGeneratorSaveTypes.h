#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Components/MInventoryComponent.h"
#include "MWorldGeneratorSaveTypes.generated.h"

USTRUCT()
struct FActorSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TSubclassOf<AActor> FinalClass;

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FString Name;
};

USTRUCT()
struct FMActorSaveData : public FActorSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	EBiome Biome;

	UPROPERTY()
	int AppearanceID;

	UPROPERTY()
	bool IsRandomizedAppearance;
};

USTRUCT()
struct FMPickableActorSaveData : public FMActorSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FItem> InventoryContents;
};

USTRUCT()
struct FMCharacterSaveData : public FActorSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName SpeciesName;

	UPROPERTY()
	float Health;
};

UCLASS()
class USaveGameWorld : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FMActorSaveData> AMActorData;

	UPROPERTY()
	TArray<FMCharacterSaveData> AMCharacterData;

	UPROPERTY()
	TArray<FMPickableActorSaveData> AMPickableActorData;
};
