#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
//#if WITH_NAKAMA
#include "NakamaError.h"
#include "NakamaNotification.h"
#include "NakamaParty.h"
#include "NakamaRPC.h"
#include "NakamaRtError.h"
#include "NakamaStorageObject.h"
#include "NakamaUnreal.h"
//#endif
#include "NakamaManager.generated.h"

using namespace NAKAMA_NAMESPACE;

#define USING_STEAM = 1

class UNakamaClient;
class UNakamaRealtimeClient;
class UNakamaSession;
class UNakamaShopManager;

DECLARE_LOG_CATEGORY_EXTERN(LogNakamaManager, Log, All);

UENUM()
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
struct FNakamaLocationMetaData
{
	GENERATED_BODY()

	UPROPERTY()
	FString city;

	UPROPERTY()
	FString country;

	UPROPERTY()
	double latitude = 0;

	UPROPERTY()
	double longitude = 0;
};

USTRUCT()
struct FNakamaUserMetaData
{
	GENERATED_BODY()

	FNakamaUserMetaData()
	    : location()
	{
	}
	UPROPERTY()
	FNakamaLocationMetaData location;
};

USTRUCT()
struct FNakamaID
{
	GENERATED_BODY()

	FNakamaID()
	    : id()
	    , nakamaId()
	{
	}

	UPROPERTY()
	FString id;
	UPROPERTY()
	FString nakamaId;
};

USTRUCT() struct FNakamaRPCErrorCode
{
	GENERATED_BODY()

	FNakamaRPCErrorCode()
	    : leaderboardShortCode()
	{
	}

	UPROPERTY()
	FString leaderboardShortCode;
};

USTRUCT()
struct FNakamaRPCErrorResponse
{
	GENERATED_BODY()

	FNakamaRPCErrorResponse()
	    : error()
	{
	}

	UPROPERTY()
	FNakamaRPCErrorCode error;
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

USTRUCT()
struct FLeaderboardEntriesJson
{
	GENERATED_BODY()

	FLeaderboardEntriesJson()
	    : leaderboards()
	    , player()
	    , social(false)
	    , inverseSocial(false)
	{
	}

	UPROPERTY()
	TArray<FString> leaderboards;
	UPROPERTY()
	FString player;
	UPROPERTY()
	bool social;
	UPROPERTY()
	bool inverseSocial;
};

USTRUCT()
struct FCurrencies
{
	GENERATED_BODY()

	FCurrencies()
	    : VC(0)
	    , XP(0)
	{
	}

	UPROPERTY()
	int32 VC;
	UPROPERTY()
	int32 XP;
};

USTRUCT()
struct FExternalIds
{
	GENERATED_BODY()

	FExternalIds()
	    : ST()
	{
	}

	UPROPERTY()
	FName ST;
};

USTRUCT()
struct FLocation
{
	GENERATED_BODY()

	FLocation()
	    : city()
	    , country()
	{
	}

	UPROPERTY()
	FString city;
	UPROPERTY()
	FString country;
};

USTRUCT()
struct FScriptData
{
	GENERATED_BODY()

	FScriptData()
	    : level(0)
	{
	}

	UPROPERTY()
	int32 level;
};

USTRUCT()
struct FAccountDetailJson
{
	GENERATED_BODY()

	FAccountDetailJson()
	    : achievements()
	    , currencies()
	    , displayName()
	    , userId()
	    , externalIds()
	    , location()
	    , scriptData()
	{
	}

	UPROPERTY()
	TArray<FName> achievements;
	UPROPERTY()
	FCurrencies currencies;
	UPROPERTY()
	FString displayName;
	UPROPERTY()
	FName userId;
	UPROPERTY()
	FExternalIds externalIds;
	UPROPERTY()
	FLocation location;
	UPROPERTY()
	FScriptData scriptData;

	//FAccountDetails ToAccountDetails() const;
};

USTRUCT()
struct FLeaderboardRankDetailsJson
{
	GENERATED_BODY()

	FLeaderboardRankDetailsJson()
	    : globalCount(0)
	    , globalFrom(0)
	    , globalFromPercent(0)
	    , globalTo(0)
	    , globalToPercent(0)
	    , socialCount(0)
	    , socialFrom(0)
	    , socialFromPercent(0)
	    , socialTo(0)
	    , socialToPercent(0)
	    //, topNPassed()
	{
	}

	UPROPERTY()
	int32 globalCount;
	UPROPERTY()
	int32 globalFrom;
	UPROPERTY()
	int32 globalFromPercent;
	UPROPERTY()
	int32 globalTo;
	UPROPERTY()
	int32 globalToPercent;

	UPROPERTY()
	int32 socialCount;
	UPROPERTY()
	int32 socialFrom;
	UPROPERTY()
	int32 socialFromPercent;
	UPROPERTY()
	int32 socialTo;
	UPROPERTY()
	int32 socialToPercent;

	//UPROPERTY()
	//TArray<FLeaderboardEntryJson> topNPassed;

	//FLeaderboardRankDetails ToLeaderboardRankDetails() const;
};

USTRUCT()
struct FLeaderboardNotification
{
	GENERATED_BODY()

	FLeaderboardNotification()
	    //: leaderboardData()
	    : leaderboardName()
	    , leaderboardShortCode()
	    , messageId()
	    , notification(false)
	    , rankDetails()
	    , subtitle()
	    , summary()
	    , title()
	{
	}

	//UPROPERTY()
	//FLeaderboardEntryJson leaderboardData;
	UPROPERTY()
	FString leaderboardName;
	UPROPERTY()
	FString leaderboardShortCode;
	UPROPERTY()
	FString messageId;
	UPROPERTY()
	bool notification;
	UPROPERTY()
	FLeaderboardRankDetailsJson rankDetails;
	UPROPERTY()
	FString subtitle;
	UPROPERTY()
	FString summary;
	UPROPERTY()
	FString title;
};

UENUM(BlueprintType)
enum class EAuthenticationMethod : uint8
{
	Server,
	Steam,		//User SteamID and Steam auth token
	Anonymous,	//A random new account
	Custom		//Use  UserID (guid) and UserName
	//Microsoft,
	//Sony
	//etc
};

UCLASS(BlueprintType, Config = NakamaManager) class NAKAMAMANAGER_API UNakamaManager : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Initialise(bool IN_IsDedicatedServer);
	bool IsInitialised() const;

	UFUNCTION(BlueprintCallable)
	bool IsAuthenticatedWithSteam() const;

	UFUNCTION(BlueprintCallable)
	bool UsingPreviewServer() const { return bUsePreviewServer; }

	UFUNCTION(BlueprintCallable, Category = "Backend")
	virtual UNakamaRealtimeClient* GetNakamaRealtimeClient() const { return this->NakamaRealtimeClient; }

	UFUNCTION(BlueprintCallable, Category = "Backend")
	virtual UNakamaClient* GetNakamaClient() const { return this->NakamaClient; }

	const FString& GetSteamID() const { return SteamID; }

private:
	// Authenticated data

	void SetAuthenticatedUserDisplayName(const FString& IN_DisplayName);
	void SetAuthenticatedUserCountry(const FString& IN_Country);
	void SetAuthenticatedUserCity(const FString& IN_City);
	virtual bool IsConnected() const;
	virtual bool IsUsingPreviewServer() const;
	const FName& GetAuthenticatedUserID() const;    // #todo: Rename/split in to cloud doc user id
	const FString& GetAuthenticatedUserDisplayName() const;
	const FString& GetAuthenticatedUserCountry() const;
	const FString& GetAuthenticatedUserCity() const;

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

	//UPROPERTY()
	//FMaintenanceStatus CachedMaintenanceStatus;

	// NAKAMA
public:
	UPROPERTY()
	UNakamaClient* NakamaClient;

	UPROPERTY(VisibleAnywhere)
	UNakamaRealtimeClient* NakamaRealtimeClient;

	UPROPERTY()
	UNakamaShopManager* NakamaShopManager;

	UPROPERTY()
	UNakamaSession* UserSession;

	UFUNCTION()
	void OnAuthenticationSuccess(UNakamaSession* IN_LoginData);

	UFUNCTION()
	void OnAuthenticationError(const FNakamaError& IN_Error);

	UFUNCTION()
	void OnSteamAuthenticationError(const FNakamaError& IN_Error);

	UFUNCTION()
	void OnAccountUpdateSucceeded();

	UFUNCTION()
	void OnAccountUpdateFailed(const FNakamaError& IN_Error);

	UFUNCTION()
	void OnRealtimeClientConnectSuccess();

	UFUNCTION()
	void OnRealtimeClientConnectError();

	//UFUNCTION()
	//void OnReceivedNotification(const FNakamaNotificationList& NotificationList);

	UFUNCTION()
	void OnRPCSuccess(const FNakamaRPC& IN_RPC);

	UFUNCTION()
	void OnRPCError(const FNakamaError& IN_Error);

	UFUNCTION()
	void OnLogEventSuccess(const FNakamaRPC& IN_RPC);

	// JSON
public:
	static FString ConvertToJson(const TMap<FString, int32>& IN_Map);

	static FString ConvertToJson(const TMap<FString, FString>& IN_Map);

	static FString GetJsonStringFromRPC(const FNakamaRPC& IN_RPC);

private:
	// UFUNCTION()
	// void OnNewHighScore(FGSNewHighScoreMessage IN_GSNewHighScoreMessage);

	void AuthenticateUsingSteam();
	void AuthenticateAnonymous();
	void AuthenticateServer();
	void AuthenticateCustom();
	bool GetSteamData(FString& OUT_ID, FString& OUT_Token, FString& OUT_NickName) const;
	//void GetMaintenanceStatus(const FOnGetMaintenanceStatusCompleteDelegate& IN_Callback);

	/*UPROPERTY()
	TArray<FAchievementEarned> AchievementsEarnedArray;

	UPROPERTY()
	TArray<FLeaderboardNewHighScore> LeaderboardNewHighScoresArray;

	UPROPERTY()
	TArray<FLeaderboardRankChanged> LeaderboardGlobalRankChanges;

	UPROPERTY()
	TArray<FLeaderboardRankChanged> LeaderboardSocialRankChanges;*/

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
