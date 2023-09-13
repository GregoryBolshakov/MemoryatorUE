#pragma once

#include "CoreMinimal.h"
#include "NakamaClient.h"

#include "UserManagerClient.generated.h"

class UNakamaManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHeroDataUpdated);

USTRUCT(BlueprintType)
struct FProfile
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Email;

	// Uncomment and define these structures if you want to use them later.
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Profile")
	// FVerified Verified;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Profile")
	// FOptins Optins;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Profile")
	// FPenaltiesApplied PenaltiesApplied;

	UPROPERTY(BlueprintReadOnly)
	FDateTime LastLogin;

	UPROPERTY(BlueprintReadOnly)
	bool RequiresReview = true;
};

USTRUCT(BlueprintType)
struct FHero
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Name;

	UPROPERTY(BlueprintReadWrite)
	int32 Level = 1;

	UPROPERTY(BlueprintReadWrite)
	int32 Experience = 0;

	// TODO: Reputation. Skills
};

/** Client that gets and updates user data */
UCLASS(BlueprintType)
class NAKAMAMANAGER_API UUserManagerClient : public UObject
{
	GENERATED_BODY()

public:
	void Initialise(UNakamaManager* IN_NakamaManager);

	UFUNCTION(BlueprintCallable)
	void RequestIfHeroExists();

	UFUNCTION(BlueprintCallable)
	void RequestCreateHero(const FString& Name, const FString& FullName, const FString& Email, bool TermsOfService, bool CompetitionRules, bool MarketingMaterial);

	/** Requests FHero data. If there is no hero, will receive a fresh one */
	UFUNCTION(BlueprintCallable)
	void RequestHeroData();

	UFUNCTION(BlueprintCallable)
	void SendHeroData();

	UFUNCTION()
	void OnHeroDataReceived(const FNakamaRPC& RPC);

	UPROPERTY(BlueprintAssignable)
	FOnRPC HeroExistsResponseDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnRPC CreateHeroResponseDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnHeroDataUpdated HeroDataUpdatedDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnRPC HeroDataSentDelegate;

	UPROPERTY(BlueprintReadWrite)
	FHero Hero;

private:
	UPROPERTY()
	UNakamaManager* NakamaManager;
};

