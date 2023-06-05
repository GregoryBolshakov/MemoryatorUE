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

class UNakamaRegistrationManager;
class UNakamaClient;
class UNakamaRealtimeClient;
class UNakamaSession;
class UNakamaPrivateLobbyManager;

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

/*USTRUCT()
struct FLeaderboardEntryJson
{
	GENERATED_BODY()

	FLeaderboardEntryJson()
	    : city()
	    , country()
	    , externalIds()
	    , rank(0)
	    , score(0)
	    , userId()
	    , userName()
	    , when()
	    , HASH()
	    , SECTOR1(0)
	    , SECTOR2(0)
	    , SECTOR3(0)
	{
	}

	UPROPERTY()
	FString city;
	UPROPERTY()
	FString country;
	UPROPERTY()
	TMap<FString, FString> externalIds;
	UPROPERTY()
	int32 rank;
	UPROPERTY()
	int32 score;
	UPROPERTY()
	FString userId;
	UPROPERTY()
	FString userName;
	UPROPERTY()
	FString when;
	UPROPERTY()
	FString HASH;
	UPROPERTY()
	int32 SECTOR1;
	UPROPERTY()
	int32 SECTOR2;
	UPROPERTY()
	int32 SECTOR3;

	FLeaderboardEntry ToLeaderboardEntry() const;
};

USTRUCT()
struct FLeaderboardEntryArrayJson
{
	GENERATED_BODY()

	FLeaderboardEntryArrayJson()
	    : leaderboardShortCode()
	    , data()
	    , first()
	    , last()
	{
	}

	UPROPERTY()
	FString leaderboardShortCode;
	UPROPERTY()
	TArray<FLeaderboardEntryJson> data;
	UPROPERTY()
	TArray<FLeaderboardEntryJson> first;
	UPROPERTY()
	TArray<FLeaderboardEntryJson> last;

	FLeaderboardData ToLeaderboardData() const;
};

USTRUCT()
struct FFriendJson
{
	GENERATED_BODY()

	FFriendJson()
	    : achievements()
	    , displayName()
	    , externalIds()
	    , id()
	{
	}

	UPROPERTY()
	TArray<FString> achievements;
	UPROPERTY()
	FString displayName;
	UPROPERTY()
	TMap<FString, FString> externalIds;
	UPROPERTY()
	FString id;

	UPROPERTY()
	bool online = false;
	// Unused
	// UPROPERTY()
	// TArray<Something> virtualGoods;
	FFriend ToFriend() const;
};

USTRUCT()
struct FFriendArrayJson
{
	GENERATED_BODY()

	FFriendArrayJson()
	    : friends()
	{
	}

	UPROPERTY()
	TArray<FFriendJson> friends;

	FFriendsList ToFriendsList() const;
};*/

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

	// Auth
	UFUNCTION(BlueprintCallable)
	bool IsAuthenticatedWithSteam() const;

	UFUNCTION(BlueprintCallable)
	bool UsingPreviewServer() const { return bUsePreviewServer; }

	UFUNCTION(BlueprintCallable, Category = "Backend")
	virtual UNakamaRealtimeClient* GetNakamaRealtimeClient() const { return this->NakamaRealtimeClient; }

	UFUNCTION(BlueprintCallable, Category = "Backend")
	virtual UNakamaClient* GetNakamaClient() const { return this->NakamaClient; }

	/*// Achievements

	virtual FOnAchievementEarnedEvent& GetOnAchievementEarnedEvent() override;
	virtual void GetAchievementsEarnedThisRuntime(TArray<FAchievementEarned>& OUT_AchievementsEarned) override;

	// Analytics

	virtual void AnalyticsRequest(const FString& IN_Key, bool IN_bStart, bool IN_bEnd, const FOnAnalyticsRequestCompleteDelegate& IN_Callback) override;
	virtual void EndSessionRequest(const FOnEndSessionRequestCompleteDelegate& IN_Callback) override;

	// Currencies

	virtual FOnXpLevelUpEvent& GetOnXpLevelUpEvent() override;
	virtual void GetXpLevelsTable(const FOnGetXPLevelsTableCompleteDelegate& IN_Callback) override;
	virtual void AddXP(int32 IN_DeltaXP, FOnXPChange IN_Callback) override;
	virtual void AddVC(int32 IN_DeltaVC, FOnVCChange IN_Callback) override;

	// Maintenance

	virtual const FMaintenanceStatus& GetCachedMaintenanceStatus() const override;
	virtual FOnMaintenanceStartedEvent& GetOnMaintenanceStartedEvent() override;
	virtual FOnMaintenanceEndedEvent& GetOnMaintenanceEndedEvent() override;

	// Storage

	UFUNCTION(BlueprintCallable)
	virtual void WriteToStorage(const FString& Collection, const FString& Key, const FString& Value);    //, const FString& Version);

	// Events
	FOnAchievementEarnedEvent Event_OnAchievementEarned;
	FOnXpLevelUpEvent Event_OnXpLevelUp;
	FOnMaintenanceStartedEvent Event_OnMaintenanceStarted;
	FOnMaintenanceEndedEvent Event_OnMaintenanceEnded;*/

private:
	// Authenticated data

	void SetAuthenticatedUserDisplayName(const FString& IN_DisplayName);
	void SetAuthenticatedUserID(const FName& IN_UserID);
	void SetAuthenticatedUserCountry(const FString& IN_Country);
	void SetAuthenticatedUserCity(const FString& IN_City);
	virtual bool IsConnected() const;
	virtual bool IsUsingPreviewServer() const;
	virtual const FName& GetAuthenticatedUserID() const;    // #todo: Rename/split in to cloud doc user id
	virtual const FName& GetCloudDocumentUserID() const;
	virtual const FString& GetAuthenticatedUserDisplayName() const;
	virtual const FString& GetAuthenticatedUserCountry() const;
	virtual const FString& GetAuthenticatedUserCity() const;
	//virtual void GetAccountDetails(const FOnGetAccountDetailsCompleteDelegate& IN_Callback);
	//virtual void GetFriendsList(const FOnGetFriendsListCompleteDelegate& IN_Callback);

	// Events

	//virtual void LogEvent(const FString& IN_EventKey, const TMap<FString, int32>& IN_Attributes, const FOnLogEventCompleteDelegate& IN_Callback) override;
	//virtual void LogEvent(const FString& IN_EventKey, const TMap<FString, FString>& IN_Attributes, const FOnLogEventCompleteDelegate& IN_Callback) override;

	// Leaderboards

	/*virtual void LeaderboardSubmitScore(const FString& IN_LeaderboardName, const TMap<FString, FString>& IN_Attributes, const FOnLeaderboardSubmitScoreCompleteDelegate& IN_Callback) override;
	virtual void GetLeaderboardData(bool IN_bGlobal,
	                                const FString& IN_LeaderboardID,
	                                int32 IN_Entries,
	                                int32 IN_Offset,
	                                const TArray<FName>& IN_FriendIDs,
	                                int32 IN_IncludeFirst,
	                                int32 IN_IncludeLast,
	                                bool IN_bSocial,
	                                bool IN_bInverseSocial,
	                                const TArray<FName>& IN_TeamIDs,
	                                const TArray<FName>& IN_TeamTypes,
	                                const FOnGetLeaderboardDataCompleteDelegate& IN_Callback) override;
	virtual void GetLeaderboardEntries(
	    bool bIsGlobal, const TArray<FString>& IN_LeaderboardIDs, const FName& IN_PlayerID, bool IN_bIncludeSocial, bool IN_bInverseSocial, const TArray<FName>& IN_TeamTypes, const FOnGetLeaderboardEntriesCompleteDelegate& IN_Callback) override;
	virtual void GetLeaderboardEntriesAroundPlayer(bool IN_bGlobal,
	                                               const FString& IN_LeaderboardID,
	                                               int32 IN_Entries,
	                                               const TArray<FName>& IN_FriendIDs,
	                                               int32 IN_IncludeFirst,
	                                               int32 IN_IncludeLast,
	                                               bool IN_bSocial,
	                                               bool IN_bInverseSocial,
	                                               const TArray<FName>& IN_TeamIDs,
	                                               const TArray<FName>& IN_TeamTypes,
	                                               const FOnGetLeaderboardEntriesAroundPlayerCompleteDelegate& IN_Callback) override;
	virtual void GetLeaderboardEntry(bool IN_bGlobal, const FString& IN_LeaderboardID, const FName& IN_PlayerID, const FOnGetLeaderboardEntryCompleteDelegate& IN_Callback) override;

	virtual void GetLeaderboardNewHighScores(TArray<FLeaderboardNewHighScore>& OUT_NewHighScores) override;
	virtual void GetLeaderboardGlobalRankChanges(TArray<FLeaderboardRankChanged>& OUT_GlobalRankChanges) override;
	virtual void GetLeaderboardSocialRankChanges(TArray<FLeaderboardRankChanged>& OUT_SocialRankChanges) override;

	FOnLeaderboardNewHighScoreEvent Event_OnLeaderboardNewHighScore;
	virtual FOnLeaderboardNewHighScoreEvent& GetOnLeaderboardNewHighScoreEvent() override;

	FOnLeaderboardRankChangedEvent Event_OnLeaderboardGlobalRankChangedEvent;
	virtual FOnLeaderboardRankChangedEvent& GetOnLeaderboardGlobalRankChangedEvent() override;

	FOnLeaderboardRankChangedEvent Event_OnLeaderboardSocialRankChangedEvent;
	virtual FOnLeaderboardRankChangedEvent& GetOnLeaderboardSocialRankChangedEvent() override;

	UFUNCTION()
	void OnWriteToStorageComplete(const FNakamaStoreObjectAcks& StorageObjectsAcks);
	UFUNCTION()
	void OnWriteToStorageError(const FNakamaError& IN_Error);*/

private:
	UPROPERTY()
	bool bIsConnected;

	UPROPERTY()
	FString UserDisplayName;

	UPROPERTY()
	FName MongoDBUserID;

	UPROPERTY()
	FName NakamaUserID;

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
	UNakamaSession* UserSession;

	UFUNCTION()
	void OnAuthenticationSuccessIDRetrieval(UNakamaSession* IN_LoginData);

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

	UFUNCTION()
	void OnGamesparksIDRetrieved(const FNakamaRPC& IN_RPC);

	// JSON
public:
	static FString ConvertToJson(const TMap<FString, int32>& IN_Map);

	static FString ConvertToJson(const TMap<FString, FString>& IN_Map);

	static FString GetJsonStringFromRPC(const FNakamaRPC& IN_RPC);

private:
	// UFUNCTION()
	// void OnNewHighScore(FGSNewHighScoreMessage IN_GSNewHighScoreMessage);

	void Authenticate();
	void AuthenticateUsingGamesparksID(FString IN_GamesparksID);
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
