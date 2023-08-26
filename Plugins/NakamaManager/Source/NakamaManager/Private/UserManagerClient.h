#pragma once

#include "CoreMinimal.h"
#include "NakamaClient.h"

#include "UserManagerClient.generated.h"

class UNakamaManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHeroDataUpdated)

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

	UPROPERTY(BlueprintReadOnly)
	FString Id;

	UPROPERTY(BlueprintReadOnly)
	FString SteamId;

	UPROPERTY(BlueprintReadOnly)
	FString Name;

	UPROPERTY(BlueprintReadOnly)
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly)
	int32 Experience = 0;

	// TODO: Reputation. Skills

	UPROPERTY(BlueprintReadOnly)
	FDateTime LastNameChange;

	UPROPERTY(BlueprintReadOnly)
	FProfile Profile;
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

	UFUNCTION()
	void OnHeroDataReceived(const FNakamaRPC& RPC);

	UFUNCTION(BlueprintCallable)
	FHero GetHero() const { return Hero; }

	UPROPERTY(BlueprintAssignable)
	FOnRPC HeroExistsResponseDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnRPC CreateHeroResponseDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnHeroDataUpdated HeroDataUpdatedDelegate;

private:
	UPROPERTY()
	UNakamaManager* NakamaManager;

	UPROPERTY()
	FHero Hero;
};

