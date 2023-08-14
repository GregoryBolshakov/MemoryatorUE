#pragma once

#include "CoreMinimal.h"
#include "MReputationManager.generated.h"

class UMInventoryComponent;
class AMCharacter;

UENUM(BlueprintType)
enum class EFaction : uint8
{
	Humans = 0,
	Nightmares,
	Witches,
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EFaction, EFaction::Count);

USTRUCT(BlueprintType)
struct FReputation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int fans;

	UPROPERTY(BlueprintReadOnly)
	int haters;
};

//TODO: This class should communicate with Nakama server to obtain/update reputation
//** The class responsible for storing and managing player's reputation points with all factions */
UCLASS(Blueprintable, BlueprintType)
class UMReputationManager : public UObject
{

public:
	GENERATED_BODY()
	void Initialize(const TMap<EFaction, FReputation>& IN_ReputationMap) { ReputationMap = IN_ReputationMap; }

	UFUNCTION(BlueprintCallable)
	FReputation GetReputation(EFaction IN_Faction);

public: //TODO: Put this to some blueprint library
	UFUNCTION()
	FString FactionToString(const EFaction Faction);

	UFUNCTION(BlueprintCallable)
	TArray<EFaction> GetAllFactions();

protected:
	/** Track 2 numbers per faction: count of creatures that think you are good/bad */
	TMap<EFaction, FReputation> ReputationMap;
};

