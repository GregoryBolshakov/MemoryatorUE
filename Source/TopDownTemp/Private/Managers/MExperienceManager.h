#pragma once

#include "CoreMinimal.h"
#include "MExperienceManager.generated.h"

class AMPickableActor;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExperienceAdded, int, Amount);

USTRUCT()
struct FMyStruct
{
	GENERATED_BODY()
	
};

//** The class responsible for managing player's experience points (current level, exp boost, etc.).
//   Doesn't store data, but takes it from Nakama User Manager Client */
UCLASS(Blueprintable, BlueprintType)
class TOPDOWNTEMP_API UMExperienceManager : public UObject
{
public:
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	int GetLevel();

	UFUNCTION(BlueprintCallable)
	int GetCurrentExperience();

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

	float ExperienceBoost = 0.f; //TODO: move to UserManagerClient's Hero

	/** Denotes experience cost for each level. Initialized using Nakama server data. Although is indexed from 0, indexes MATCH levels */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=MExperienceManager)
	TArray<int> LevelsCost;

	/** Denotes experience reward per each MPickableActor. Initialized using Nakama server data. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=MExperienceManager)
	TMap<TSubclassOf<AMPickableActor>, int> ExperiencePerPickUp;
};
