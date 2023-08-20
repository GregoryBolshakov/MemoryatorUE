#pragma once

#include "CoreMinimal.h"
#include "MExperienceManager.generated.h"

class AMPickableActor;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExperienceAdded, int, Amount)

USTRUCT()
struct FMyStruct
{
	GENERATED_BODY()
	
};

//TODO: This class should communicate with Nakama server to limit experience growth over time
//** The class responsible for managing player's experience points (current level, exp boost, etc.) */
UCLASS(Blueprintable, BlueprintType)
class TOPDOWNTEMP_API UMExperienceManager : public UObject
{
public:
	GENERATED_BODY()

	void Initialize();

	UFUNCTION(BlueprintCallable)
	int GetLevel() { return Level; }

	UFUNCTION(BlueprintCallable)
	int GetCurrentExperience() { return CurrentExperience; }

	UFUNCTION(BlueprintCallable)
	float GetExperiencePercent();

	UFUNCTION()
	void AddExperience(int Addition);

	/** Experience reward for picking some item/actor up */
	UFUNCTION()
	void OnActorPickedUp(TSubclassOf<AMPickableActor> IN_Class);

	UPROPERTY(BlueprintAssignable)
	FOnExperienceAdded ExperienceAddedDelegate;

protected:
	int Level = 1;

	int CurrentExperience = 0.f;

	float ExperienceBoost = 0.f;

	/** Denotes experience cost for each level. Initialized using Nakama server data. Although is indexed from 0, indexes MATCH levels */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=MExperienceManager)
	TArray<int> LevelsCost;

	/** Denotes experience reward per each MPickableActor. Initialized using Nakama server data. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=MExperienceManager)
	TMap<TSubclassOf<AMPickableActor>, int> ExperiencePerPickUp;
};
