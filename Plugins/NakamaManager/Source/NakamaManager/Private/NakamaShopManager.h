#pragma once

#include "CoreMinimal.h"
#include "NakamaManager.h"

#include "NakamaShopManager.generated.h"

class UNakamaClient;
class UNakamaSession;
class UNakamaManager;
struct FNakamaRPC;

USTRUCT()
struct FShopItem
{
	GENERATED_BODY()

	uint32 itemid;
  
	int16 qty;

	int64 amount;

	FString description;

	FString category;
};

USTRUCT()
struct FBundle
{
	GENERATED_BODY()

	uint32 itemcount;

	TArray<FShopItem> items;
};

USTRUCT()
struct FBundleByID
{
	GENERATED_BODY()

	uint32 bundleid;

	FBundle bundle;
};

USTRUCT()
struct FPayload
{
	GENERATED_BODY()

	FString steamid;

	FString language;

	FString currency;

	uint32 bundleid;

	bool issandbox;
};

UCLASS()
class NAKAMAMANAGER_API UNakamaShopManager : public UObject
{
	GENERATED_BODY()

public:

	void Initialise(UNakamaClient* IN_NakamaClient, UNakamaSession* IN_UserSession, UNakamaManager* IN_NakamaManager);

	void BuyBundle(const uint32 BundleID);

	const TMap<uint32, FBundle>& GetBundles() { return Bundles; }

private:

	UFUNCTION()
	void OnReceivedAllBundles(const FNakamaRPC& RPC);

	UPROPERTY()
	UNakamaClient* NakamaClient;

	UPROPERTY()
	UNakamaSession* UserSession;

	UPROPERTY()
	UNakamaManager* NakamaManager;

	TMap<uint32, FBundle> Bundles;
};

