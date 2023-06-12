#pragma once

#include <steam/isteamuser.h>
#include <steam/steam_api_common.h>

#include "CoreMinimal.h"
#include "NakamaManager.h"

#include "NakamaShopManager.generated.h"

class UNakamaClient;
class UNakamaSession;
class UNakamaManager;
struct FNakamaRPC;

DECLARE_DELEGATE_OneParam(FOnBundleTxnFinalized, const FBundle& Bundle);

USTRUCT()
struct FShopItem
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 itemid;

	UPROPERTY()
	int16 qty;

	UPROPERTY()
	int64 amount;

	UPROPERTY()
	FString description;

	UPROPERTY()
	FString category;
};

USTRUCT()
struct FBundle
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 itemcount;

	UPROPERTY()
	TArray<FShopItem> items;
};

USTRUCT()
struct FBundleByID
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 bundleid;

	UPROPERTY()
	FBundle bundle;
};

USTRUCT()
struct FPayload
{
	GENERATED_BODY()

	UPROPERTY()
	FString steamid;

	UPROPERTY()
	FString language;

	UPROPERTY()
	FString currency;

	UPROPERTY()
	uint32 bundleid;

	UPROPERTY()
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

	FOnBundleTxnFinalized OnBundleTxnFinalizedDelegate;

private:

	UFUNCTION()
	void OnReceivedAllBundles(const FNakamaRPC& RPC);

#ifdef USING_STEAM
	STEAM_CALLBACK(UNakamaShopManager, OnMicroTxnAuthorizationResponse, MicroTxnAuthorizationResponse_t);
#endif

	UFUNCTION()
	void OnTxnFinalized(const FNakamaRPC& RPC);

	UPROPERTY()
	UNakamaClient* NakamaClient;

	UPROPERTY()
	UNakamaSession* UserSession;

	UPROPERTY()
	UNakamaManager* NakamaManager;

	TMap<uint32, FBundle> Bundles;
};

