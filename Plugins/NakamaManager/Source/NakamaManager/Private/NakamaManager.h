#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
//#if WITH_NAKAMA
#include "NakamaError.h"
#include "NakamaParty.h"
#include "NakamaRPC.h"
//#endif
#include "NakamaManager.generated.h"

class UUserManagerClient;
using namespace NAKAMA_NAMESPACE;

#define USING_STEAM = 1

class UNakamaClient;
class UNakamaRealtimeClient;
class UNakamaSession;
class UShopManagerClient;

DECLARE_LOG_CATEGORY_EXTERN(LogNakamaManager, Log, All);

UENUM(BlueprintType)
enum class ENakamaInitialisationStatus : uint8
{
	Uninitialised,
	Initialising,
	DownForMainenance,
	ErrorNoConnection,
	ErrorFailedToAuthenticate,
	ErrorGetAccountDetailsFailed,
	ErrorGetMaintenanceStatusFailed,
	Initialised
};
USTRUCT()
struct FNakamaPayloadWithJson
{
	GENERATED_BODY()

	FNakamaPayloadWithJson()
	    : JSONObject()
	{
	}

	UPROPERTY()
	FString JSONObject;
};

UENUM(BlueprintType)
enum class EAuthenticationMethod : uint8
{
	Server,
	Steam,		//User SteamID and Steam auth token
	//Microsoft,
	//Sony
	//etc
};

UCLASS(BlueprintType, Config = NakamaManager) class NAKAMAMANAGER_API UNakamaManager : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Initialise(bool IN_IsDedicatedServer);

	UFUNCTION(BlueprintCallable)
	bool UsingPreviewServer() const { return bUsePreviewServer; }

	UFUNCTION(BlueprintCallable, Category = "Backend")
	virtual UNakamaRealtimeClient* GetNakamaRealtimeClient() const { return this->NakamaRealtimeClient; }

	UFUNCTION(BlueprintCallable, Category = "Backend")
	virtual UNakamaClient* GetNakamaClient() const { return this->NakamaClient; }

	const FString& GetSteamID() const { return SteamID; }

	UFUNCTION(BlueprintCallable)
	ENakamaInitialisationStatus GetInitialisationStatus() const { return InitialisationStatus; }

private:
	UPROPERTY()
	bool bIsConnected;

	UPROPERTY()
	FString UserDisplayName;

	UPROPERTY()
	FName NakamaUserID;

	UPROPERTY()
	FString SteamID;

	UPROPERTY()
	FString UserCountry;

	UPROPERTY()
	FString UserCity;

	UPROPERTY(Config)
	int32 ConnectionRetriesMax;

	UPROPERTY(Config)
	int32 AuthenticationRetriesMax;

	UPROPERTY(Config)
	bool bUsePreviewServer;

	// If true, we try to deduct currency for purchases before proceeding. Otherwise, all purchases are allowed without deduction.
	UPROPERTY(Config)
	bool bVCChangeAllowed;

	// Should be a guid
	UPROPERTY(Config)
	FString UserID;

	UPROPERTY(Config)
	FString UserName;

	UPROPERTY(Config)
	FString UserPassword;

	UPROPERTY(Config)
	FString ServerUserName;

	UPROPERTY(Config)
	FString ServerUserPassword;

	// NAKAMA
public:
	UPROPERTY()
	UNakamaClient* NakamaClient;

	UPROPERTY(VisibleAnywhere)
	UNakamaRealtimeClient* NakamaRealtimeClient;

	UPROPERTY(BlueprintReadOnly)
	UUserManagerClient* UserManagerClient;

	UPROPERTY(BlueprintReadOnly)
	UShopManagerClient* ShopManagerClient;

	UPROPERTY()
	UNakamaSession* UserSession;

	UFUNCTION()
	void OnAuthenticationSuccess(UNakamaSession* IN_LoginData);

	UFUNCTION()
	void OnSteamAuthenticationError(const FNakamaError& IN_Error);

	UFUNCTION()
	void OnRPCSuccess(const FNakamaRPC& IN_RPC);

	UFUNCTION()
	void OnRPCError(const FNakamaError& IN_Error);

	// JSON
public:
	static FString ConvertToJson(const TMap<FString, int32>& IN_Map);

	static FString ConvertToJson(const TMap<FString, FString>& IN_Map);

	static FString GetJsonStringFromRPC(const FNakamaRPC& IN_RPC);

private:
	void AuthenticateUsingSteam();
	bool GetSteamData(FString& OUT_ID, FString& OUT_Token, FString& OUT_NickName) const;

	UPROPERTY(Config)
	FString PreviewServerKey;

	UPROPERTY(Config)
	FString ProductionServerKey;

	UPROPERTY(Config)
	FString PreviewHost;

	UPROPERTY(Config)
	FString ProductionHost;

	ENakamaInitialisationStatus InitialisationStatus;
	bool bIsDedicatedServer;
	int32 SteamAuthenticationRetries;
};
