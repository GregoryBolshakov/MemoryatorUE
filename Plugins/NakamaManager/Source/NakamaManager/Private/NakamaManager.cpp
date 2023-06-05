#include "NakamaManager.h"

#include "JsonObjectConverter.h"
#include "NakamaClient.h"
#include <Templates/SharedPointer.h>

#if PLATFORM_WINDOWS
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemSteam.h"
#endif

DEFINE_LOG_CATEGORY(LogNakamaManager);

//PRAGMA_DISABLE_OPTIMIZATION

bool JsonObjectToString(const TSharedPtr<FJsonObject> IN_JsonObject, FString& OUT_JsonObjectString)
{
	const TSharedRef<FJsonObject> ItemRef = IN_JsonObject.ToSharedRef();
	TSharedRef<TJsonWriter<>> JsonWriterArgs = TJsonWriterFactory<>::Create(&OUT_JsonObjectString);
	auto result = FJsonSerializer::Serialize(ItemRef, JsonWriterArgs);
	if (!result)
	{
		UE_LOG(LogNakamaManager, Log, TEXT("Unable to get a string representation of the Json Object."));
	}
	JsonWriterArgs->Close();
	return result;
}

bool StringToFDateTime(const FString& IN_DateTimeString, FDateTime& OUT_DateTime)
{
	bool bDateTimeParsedSuccessfully = FDateTime::ParseIso8601(*IN_DateTimeString, OUT_DateTime);
	if (bDateTimeParsedSuccessfully)
	{
		return true;
	}
	FString GSDateTimeString = IN_DateTimeString;

	int32 ZIndex(INDEX_NONE);
	bool bFoundZ = GSDateTimeString.FindLastChar(TCHAR('Z'), ZIndex);
	ensure(bFoundZ);
	if (!bFoundZ)
	{
		return false;
	}
	ensure(0 < ZIndex);

	GSDateTimeString.InsertAt(ZIndex - 1, ":00");

	bDateTimeParsedSuccessfully = FDateTime::ParseIso8601(*GSDateTimeString, OUT_DateTime);
	ensure(bDateTimeParsedSuccessfully);

	return bDateTimeParsedSuccessfully;
}

/*FLeaderboardEntry FLeaderboardEntryJson::ToLeaderboardEntry() const
{
	FLeaderboardEntry Result;
	Result.City = FName(*this->city);
	Result.Country = FName(*this->country);
	Result.DateTime = FDateTime::UtcNow();
	StringToFDateTime(this->when, Result.DateTime);
	Result.Rank = this->rank;
	Result.Score = this->score;
	Result.Sector1 = this->SECTOR1;
	Result.Sector2 = this->SECTOR2;
	Result.Sector3 = this->SECTOR3;
	Result.UserID = FName(*this->userId);    // IInterfaceBackendService::ToMongoDBUserID(FName(*this->userId));
	Result.UserName = FName(*this->userName);
	Result.SteamID = FName(*this->externalIds.begin()->Value);
	return Result;
}

FLeaderboardData FLeaderboardEntryArrayJson::ToLeaderboardData() const
{
	FLeaderboardData Result;
	for (const FLeaderboardEntryJson& Entry : this->data)
	{
		Result.Entries.Add(Entry.ToLeaderboardEntry());
	}
	for (const FLeaderboardEntryJson& Entry : this->first)
	{
		Result.First.Add(Entry.ToLeaderboardEntry());
	}
	for (const FLeaderboardEntryJson& Entry : this->last)
	{
		Result.Last.Add(Entry.ToLeaderboardEntry());
	}

	return Result;
}

FAccountDetails FAccountDetailJson::ToAccountDetails() const
{
	FAccountDetails Result;
	Result.Achievements = achievements;
	Result.City = location.city;
	Result.Country = location.country;
	Result.DisplayName = displayName;
	Result.Level = scriptData.level;
	Result.UserID = userId;
	Result.VC = (float)currencies.VC;
	Result.XP = currencies.XP;

	return Result;
}

FFriend FFriendJson::ToFriend() const
{
	FFriend Result;
	Result.bIsOnline = this->online;
	Result.DisplayName = this->displayName;
	Result.PlayerID = FName(*this->id);
	return Result;
}

FFriendsList FFriendArrayJson::ToFriendsList() const
{
	FFriendsList Result;
	for (FFriendJson F : friends)
	{
		Result.Friends.Add(F.ToFriend());
	}
	return Result;
}*/

FName ToNakamaUserID(FName IN_UserID)
{
	FString UserIDString = IN_UserID.ToString();
	if (UserIDString.Len() == 24)    // Hack: Detected a mongodb id, so convert to nakama by prepending zeros
	{
		UserIDString = TEXT("00000000") + UserIDString;
	}

	return FName(*UserIDString);
}

FString ToNakamaUserIDString(const FString& IN_UserID)
{
	FString UserIDString = IN_UserID;
	if (UserIDString.Len() == 24)    // Hack: Detected a mongodb id, so convert to nakama by prepending zeros
	{
		UserIDString = TEXT("00000000") + UserIDString;
	}

	return UserIDString;
}

FString UNakamaManager::ConvertToJson(const TMap<FString, int32>& IN_Map)
{
	int32 Count = 0;
	FString FunctionPayload = TEXT("{");
	for (auto& KVP : IN_Map)
	{
		FunctionPayload += FString::Printf(TEXT("%s\"%s\": %d"), ++Count > 1 ? TEXT(",") : TEXT(""), *KVP.Key, KVP.Value);
	}
	FunctionPayload += TEXT("}");
	return FunctionPayload;
}

FString UNakamaManager::ConvertToJson(const TMap<FString, FString>& IN_Map)
{
	int32 Count = 0;
	FString FunctionPayload = TEXT("{");
	for (auto& KVP : IN_Map)
	{
		FunctionPayload += FString::Printf(TEXT("%s\"%s\": \"%s\""), ++Count > 1 ? TEXT(",") : TEXT(""), *KVP.Key, *KVP.Value);
	}
	FunctionPayload += TEXT("}");

	// Remove quotes around bools
	FunctionPayload = FunctionPayload.Replace(TEXT("\"true\""), TEXT("true"));
	FunctionPayload = FunctionPayload.Replace(TEXT("\"false\""), TEXT("false"));

	return FunctionPayload;
}

FString UNakamaManager::GetJsonStringFromRPC(const FNakamaRPC& IN_RPC)
{
	FNakamaPayloadWithJson Json;
	bool bSuccess = FJsonObjectConverter::JsonObjectStringToUStruct(IN_RPC.Payload, &Json, 0, 0);
	return bSuccess ? Json.JSONObject : TEXT("");
	return "";
}

UNakamaManager::UNakamaManager(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bIsConnected(false)
    , AuthenticationRetriesMax(5)
    , bUsePreviewServer(true)
    , bVCChangeAllowed(false)
    //, CachedMaintenanceStatus(FMaintenanceStatus())
    , PreviewServerKey(TEXT("1hP9lwynAatdvFC5"))
    , ProductionServerKey(TEXT("vrTxrvK441T3O6bk"))
    , PreviewHost(TEXT("kartkraft.as-neast3-a.nakamacloud.io"))
    , ProductionHost(TEXT("kartkraft-prod.us-east1-c.nakamacloud.io"))
    , InitialisationStatus(ENakamaInitialisationStatus::Uninitialised)
    , bIsDedicatedServer(false)
{
	LoadConfig();

	if (!HasAllFlags(RF_ClassDefaultObject))
	{
	}

	SteamAuthenticationRetries = AuthenticationRetriesMax;
}

void UNakamaManager::Initialise(bool IN_IsDedicatedServer)
{
	InitialisationStatus = ENakamaInitialisationStatus::Initialising;

	FParse::Bool(FCommandLine::Get(), TEXT("-GSPreview="), bUsePreviewServer);    //#hack Hijack gamesparks variable until we ship

	UE_LOG(LogNakamaManager, Log, TEXT("Using preview server: %s"), bUsePreviewServer ? TEXT("true") : TEXT("false"));

	bIsDedicatedServer = IN_IsDedicatedServer;
	const int32 Port = 7349;
	const bool bUseSSL = true;
	const bool bEnableDebug = true;
	const ENakamaClientType ClientType = ENakamaClientType::DEFAULT;
	const float TickInterval = 0.05f;
	const FString ClientName = IN_IsDedicatedServer ? FString(TEXT("MemoryatorServer")) : FString(TEXT("MemoryatorClient"));
	const FString ServerKey = bUsePreviewServer ? PreviewServerKey : ProductionServerKey;
	const FString Host = bUsePreviewServer ? PreviewHost : ProductionHost;
	UE_LOG(LogNakamaManager, Log, TEXT("Initialising %s using %s:%d"), *ClientName, *Host, Port);

	// Setup Default Client
	//NakamaClient = UNakamaClient::CreateDefaultClient(ServerKey, Host, Port, bUseSSL, bEnableDebug, ClientType, TickInterval, ClientName);
	NakamaClient = UNakamaClient::CreateDefaultClient("defaultkey", "localhost", 7350, false, true, ClientType, 0.02, "MyGame");

	const EAuthenticationMethod AuthMethod = IN_IsDedicatedServer ? EAuthenticationMethod::Server : EAuthenticationMethod::Steam; // TODO: Add AppleID

	UE_LOG(LogNakamaManager, Log, TEXT("Authenticating using auth method: %s"), *(UEnum::GetValueAsName(AuthMethod).ToString()));

	switch (AuthMethod)
	{
		case EAuthenticationMethod::Server:
			MongoDBUserID = TEXT("5d2d5a4a11e274051fdee748");
			UserDisplayName = TEXT("DedicatedServerUserName1");
			InitialisationStatus = ENakamaInitialisationStatus::Initialised;    //#hack no server auth for now. Having trouble getting it to work on Linux.
			// AuthenticateServer();
			break;
		case EAuthenticationMethod::Steam:
			AuthenticateUsingSteam();
			break;
		case EAuthenticationMethod::Anonymous:
			AuthenticateAnonymous();
			break;
		case EAuthenticationMethod::Custom:
			AuthenticateCustom();
			break;
			// 		case EAuthenticationMethod::Microsoft:
			// 			AuthenticateUsingMicrosoft();
			// 			break;
			// 		case EAuthenticationMethod::Sony:
			// 			AuthenticateUsingSony();
			// 			break;
		default:
			ensure(false);
			break;
	};

	// Add listeners for events

	// 	ensure(GameSparksMessageListeners);
	// 	GameSparksMessageListeners->OnNewHighScoreMessage.AddDynamic(this, &UGameSparksManager::OnNewHighScore);
	// 	GameSparksMessageListeners->OnGlobalRankChangedMessage.AddDynamic(this, &UGameSparksManager::OnGlobalRankChanged);
	// 	GameSparksMessageListeners->OnSocialRankChangedMessage.AddDynamic(this, &UGameSparksManager::OnSocialRankChanged);
	// 	GameSparksMessageListeners->OnAchievementEarnedMessage.AddDynamic(this, &UGameSparksManager::OnAchievementEarned);
	// 	GameSparksMessageListeners->OnScriptMessage.AddDynamic(this, &UGameSparksManager::OnScriptMessage);*/
}

bool UNakamaManager::IsInitialised() const { return InitialisationStatus > ENakamaInitialisationStatus::Initialising; }

bool UNakamaManager::IsAuthenticatedWithSteam() const { return false;/*return IsValid(UserSession);*//* && GetDefault<UBackendServiceSettings>()->AuthenticationMethod == EAuthenticationMethod::Steam;*/ }

/*FOnAchievementEarnedEvent& UNakamaManager::GetOnAchievementEarnedEvent() { return Event_OnAchievementEarned; }

void UNakamaManager::GetAchievementsEarnedThisRuntime(TArray<FAchievementEarned>& OUT_AchievementsEarned)
{
	ensure(0 == OUT_AchievementsEarned.Num());
	OUT_AchievementsEarned.Empty();
	OUT_AchievementsEarned.Append(AchievementsEarnedArray);
}

void UNakamaManager::AnalyticsRequest(const FString& IN_Key, bool IN_bStart, bool IN_bEnd, const FOnAnalyticsRequestCompleteDelegate& IN_Callback)
{
	const FString Function = TEXT("AnalyticsRequest");
	const FString Payload = ConvertToJson({{TEXT("key"), IN_Key}, {TEXT("start"), IN_bStart ? TEXT("true") : TEXT("false")}, {TEXT("end"), IN_bEnd ? TEXT("true") : TEXT("false")}});

	FOnRPCDelegate OnSuccess = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& IN_RPC)
	    {
		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *IN_RPC.Id, *IN_RPC.Payload);
		    IN_Callback.ExecuteIfBound(true);
	    });
	FOnErrorDelegate OnError = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& Error)
	    {
		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(false);
	    });

	NakamaClient->RPC(UserSession, Function, Payload, OnSuccess, OnError);
}

void UNakamaManager::EndSessionRequest(const FOnEndSessionRequestCompleteDelegate& IN_Callback)
{
	if (NakamaRealtimeClient) NakamaRealtimeClient->Disconnect();
	if (NakamaClient) NakamaClient->Disconnect();
	IN_Callback.ExecuteIfBound(0.0f);
}*/

void UNakamaManager::SetAuthenticatedUserDisplayName(const FString& IN_DisplayName) { UserDisplayName = IN_DisplayName; }

void UNakamaManager::SetAuthenticatedUserID(const FName& IN_UserID) { MongoDBUserID = IN_UserID; }

void UNakamaManager::SetAuthenticatedUserCountry(const FString& IN_Country) { UserCountry = IN_Country; }

void UNakamaManager::SetAuthenticatedUserCity(const FString& IN_City) { UserCity = IN_City; }

bool UNakamaManager::IsConnected() const { return bIsConnected; }

bool UNakamaManager::IsUsingPreviewServer() const { return bUsePreviewServer; }

const FName& UNakamaManager::GetAuthenticatedUserID() const { return NakamaUserID; }
const FName& UNakamaManager::GetCloudDocumentUserID() const { return MongoDBUserID; }

const FString& UNakamaManager::GetAuthenticatedUserDisplayName() const { return UserDisplayName; }

const FString& UNakamaManager::GetAuthenticatedUserCountry() const { return UserCountry; }

const FString& UNakamaManager::GetAuthenticatedUserCity() const { return UserCity; }

/*void UNakamaManager::GetAccountDetails(const FOnGetAccountDetailsCompleteDelegate& IN_Callback)
{
	if (!UserSession)
	{
		FAccountDetails OfflineAccount;
		OfflineAccount.DisplayName = TEXT("OfflinePlayer");
		IN_Callback.ExecuteIfBound(false, OfflineAccount);
		return;
	}

	const FString Function = TEXT("AccountDetailsRequest");
	const FString Payload = TEXT("");

	FOnRPCDelegate OnSuccess = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& IN_RPC)
	    {
		    FAccountDetailJson Json;
		    bool bSucces = FJsonObjectConverter::JsonObjectStringToUStruct(IN_RPC.Payload, &Json, 0, 0);
		    ensure(bSucces);

		    const FAccountDetails AccountDetails = bSucces ? Json.ToAccountDetails() : FAccountDetails();

		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *IN_RPC.Id, *IN_RPC.Payload);

		    IN_Callback.ExecuteIfBound(bSucces, AccountDetails);
	    });
	FOnErrorDelegate OnError = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& Error)
	    {
		    UE_LOG(LogNakamaManager, Log, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(false, FAccountDetails());
	    });

	NakamaClient->RPC(UserSession, Function, Payload, OnSuccess, OnError);
}

void UNakamaManager::GetFriendsList(const FOnGetFriendsListCompleteDelegate& IN_Callback)
{
	const FString Function = TEXT("ListGameFriendsRequest");
	const FString Payload = TEXT("");

	FOnRPCDelegate OnSuccess = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& IN_RPC)
	    {
		    FFriendArrayJson Json;
		    bool bSuccess = FJsonObjectConverter::JsonObjectStringToUStruct(IN_RPC.Payload, &Json, 0, 0);
		    ensure(bSuccess);

		    const FFriendsList FriendsList = bSuccess ? Json.ToFriendsList() : FFriendsList();

		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *IN_RPC.Id, *IN_RPC.Payload);
		    //
		    IN_Callback.ExecuteIfBound(bSuccess, FriendsList);
	    });
	FOnErrorDelegate OnError = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& Error)
	    {
		    UE_LOG(LogNakamaManager, Log, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(false, FFriendsList());
	    });

	NakamaClient->RPC(UserSession, Function, Payload, OnSuccess, OnError);
}

FOnXpLevelUpEvent& UNakamaManager::GetOnXpLevelUpEvent() { return Event_OnXpLevelUp; }

void UNakamaManager::GetXpLevelsTable(const FOnGetXPLevelsTableCompleteDelegate& IN_Callback)
{
	const FString Function = TEXT("GET_XP_LEVELS_TABLE");
	const FString Payload = ConvertToJson(TMap<FString, int32>());
	FOnRPCDelegate OnSuccess = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& IN_RPC)
	    {
		    FXpLevelsTable XpLevelsTable;
		    bool bSucces = FJsonObjectConverter::JsonObjectStringToUStruct(GetJsonStringFromRPC(IN_RPC), &XpLevelsTable, 0, 0);
		    ensure(bSucces);

		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *IN_RPC.Id, *IN_RPC.Payload);

		    IN_Callback.ExecuteIfBound(bSucces, XpLevelsTable);
	    });
	FOnErrorDelegate OnError = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& Error)
	    {
		    UE_LOG(LogNakamaManager, Log, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(false, FXpLevelsTable());
	    });

	NakamaClient->RPC(UserSession, Function, Payload, OnSuccess, OnError);
}

void UNakamaManager::AddXP(int32 IN_DeltaXP, FOnXPChange IN_Callback)
{
	const FString Function = TEXT("ADD_XP");
	const FString Payload = ConvertToJson({{TEXT("AMOUNT"), IN_DeltaXP}});
	FOnRPCDelegate OnSuccess = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& IN_RPC)
	    {
		    FCurrencyResult CurrencyResult;
		    bool bSucces = FJsonObjectConverter::JsonObjectStringToUStruct(GetJsonStringFromRPC(IN_RPC), &CurrencyResult, 0, 0);
		    ensure(bSucces);

		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *IN_RPC.Id, *IN_RPC.Payload);

		    IN_Callback.ExecuteIfBound(bSucces, CurrencyResult);
	    });
	FOnErrorDelegate OnError = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& Error)
	    {
		    UE_LOG(LogNakamaManager, Log, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(false, FCurrencyResult());
	    });

	NakamaClient->RPC(UserSession, Function, Payload, OnSuccess, OnError);
}

void UNakamaManager::AddVC(int32 IN_DeltaVC, FOnVCChange IN_Callback)
{
	// Reset vc change to 0 if not allowed
	if (!bVCChangeAllowed)
	{
		IN_DeltaVC = 0;
	}

	const FString Function = TEXT("ADD_VC");
	const FString Payload = ConvertToJson({{TEXT("AMOUNT"), IN_DeltaVC}});
	FOnRPCDelegate OnSuccess = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& IN_RPC)
	    {
		    FCurrencyResult CurrencyResult;
		    bool bSucces = FJsonObjectConverter::JsonObjectStringToUStruct(GetJsonStringFromRPC(IN_RPC), &CurrencyResult, 0, 0);
		    ensure(bSucces);

		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *IN_RPC.Id, *IN_RPC.Payload);

		    IN_Callback.ExecuteIfBound(bSucces, CurrencyResult);
	    });
	FOnErrorDelegate OnError = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& Error)
	    {
		    UE_LOG(LogNakamaManager, Log, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(false, FCurrencyResult());
	    });

	NakamaClient->RPC(UserSession, Function, Payload, OnSuccess, OnError);
}

void UNakamaManager::LogEvent(const FString& IN_EventKey, const TMap<FString, int32>& IN_Attributes, const FOnLogEventCompleteDelegate& IN_Callback)
{
	FOnRPC RPCSuccessDelegate;
	RPCSuccessDelegate.AddDynamic(this, &UNakamaManager::OnLogEventSuccess);

	FOnError RPCErrorDelegate;
	RPCErrorDelegate.AddDynamic(this, &UNakamaManager::OnRPCError);

	NakamaClient->RPC(UserSession, IN_EventKey, ConvertToJson(IN_Attributes), RPCSuccessDelegate, RPCErrorDelegate);
}

void UNakamaManager::LogEvent(const FString& IN_EventKey, const TMap<FString, FString>& IN_Attributes, const FOnLogEventCompleteDelegate& IN_Callback)
{
	TMap<FString, FString> ConvertedAttributes = IN_Attributes;
	const FString IDKey = TEXT("PLAYER_ID");
	// Update id if it is not in nakama form
	if (ConvertedAttributes.Contains(IDKey))
	{
		FString NewID = ToNakamaUserIDString(ConvertedAttributes[IDKey]);
		ConvertedAttributes.Add(IDKey, NewID);
	}

	const FString Function = IN_EventKey;
	const FString Payload = ConvertToJson(ConvertedAttributes);

	FOnRPCDelegate OnSuccess = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& RPC)
	    {
		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *RPC.Id, *RPC.Payload);

		    FString JSONString = GetJsonStringFromRPC(RPC);
		    if (Function == TEXT("GET_PLAYER_DETAILS") && (JSONString.IsEmpty() || JSONString == TEXT("{}")))
		    {
			    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
			    JsonObject->SetBoolField("bHasScheduledEventsAccess", true);
			    JsonObject->SetStringField("DisplayName", UserDisplayName.IsEmpty() ? UserName : UserDisplayName);
			    JsonObject->SetBoolField("EmailRegistered", false);
			    JsonObject->SetStringField("PlayerID", NakamaUserID.ToString());
			    JsonObject->SetNumberField("PreferredRaceNumber", 99);
			    JsonObject->SetStringField("Tagline", TEXT("The time for talk is over!"));

			    bool bConverted = JsonObjectToString(JsonObject, JSONString);
			    ensure(bConverted);
		    }

		    IN_Callback.ExecuteIfBound(true, JSONString);
	    });
	FOnErrorDelegate OnError = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& Error)
	    {
		    UE_LOG(LogNakamaManager, Log, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(false, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
	    });

	NakamaClient->RPC(UserSession, Function, Payload, OnSuccess, OnError);
}

void UNakamaManager::LeaderboardSubmitScore(const FString& IN_LeaderboardName, const TMap<FString, FString>& IN_Attributes, const FOnLeaderboardSubmitScoreCompleteDelegate& IN_Callback)
{
	//#todo
	const FString Function = IN_LeaderboardName;
	const FString Payload = ConvertToJson(IN_Attributes);

	FOnRPCDelegate OnSuccess = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& RPC)
	    {
		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *RPC.Id, *RPC.Payload);
		    IN_Callback.ExecuteIfBound(true);
	    });
	FOnErrorDelegate OnError = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& Error)
	    {
		    UE_LOG(LogNakamaManager, Log, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(false);
	    });

	NakamaClient->RPC(UserSession, Function, Payload, OnSuccess, OnError);
}

void UNakamaManager::GetLeaderboardData(bool IN_bGlobal,
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
                                        const FOnGetLeaderboardDataCompleteDelegate& IN_Callback)
{
	//#todo
	const FString Function = TEXT("LeaderboardDataRequest");
	const FString Payload = FString::Printf(TEXT("{\"leaderboardShortCode\" : \"%s\", \"dontErrorOnNotSocial\" : false, \"entryCount\" : %d, \"offset\" : %d, \"includeFirst\" : %d, \"includeLast\" : %d, \"social\" : %s, \"inverseSocial\": %s }"),
	                                        *IN_LeaderboardID,
	                                        IN_Entries,
	                                        IN_Offset,
	                                        IN_IncludeFirst,
	                                        IN_IncludeLast,
	                                        IN_bSocial ? TEXT("true") : TEXT("false"),
	                                        IN_bInverseSocial ? TEXT("true") : TEXT("false"));

	FOnRPCDelegate OnSuccess = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& IN_RPC)
	    {
		    FLeaderboardEntryArrayJson Json;
		    bool bSucces = FJsonObjectConverter::JsonObjectStringToUStruct(IN_RPC.Payload, &Json, 0, 0);

		    ECallbackResponseLeaderboardData Result = ECallbackResponseLeaderboardData::Success;
		    FLeaderboardData LeaderboardData = Json.ToLeaderboardData();

		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *IN_RPC.Id, *IN_RPC.Payload);
		    IN_Callback.ExecuteIfBound(Result, LeaderboardData);
	    });
	FOnErrorDelegate OnError = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& Error)
	    {
		    ECallbackResponseLeaderboardData Result = ECallbackResponseLeaderboardData::ErrorUnspecified;
		    FLeaderboardData LeaderboardData;

		    UE_LOG(LogNakamaManager, Log, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(Result, LeaderboardData);
	    });

	NakamaClient->RPC(UserSession, Function, Payload, OnSuccess, OnError);
}

void UNakamaManager::GetLeaderboardEntries(
    bool bIsGlobal, const TArray<FString>& IN_LeaderboardIDs, const FName& IN_PlayerID, bool IN_bIncludeSocial, bool IN_bInverseSocial, const TArray<FName>& IN_TeamTypes, const FOnGetLeaderboardEntriesCompleteDelegate& IN_Callback)
{
	//#todo

	FLeaderboardEntriesJson RequestData;
	RequestData.leaderboards = IN_LeaderboardIDs;
	RequestData.player = ToNakamaUserID(IN_PlayerID).ToString();
	RequestData.social = IN_bIncludeSocial;
	RequestData.inverseSocial = IN_bInverseSocial;

	FString JsonString;

	bool bSucces = FJsonObjectConverter::UStructToJsonObjectString(RequestData, JsonString);
	ensure(bSucces);

	const FString Function = TEXT("GetLeaderboardEntriesRequest");
	const FString Payload = JsonString;

	FOnRPCDelegate OnSuccess = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& RPC)
	    {
		    ECallbackResponseLeaderboardEntries Result = ECallbackResponseLeaderboardEntries::Success;
		    FLeaderboardEntries LeaderboardEntries;
		    //#todo: Implement

		    TSharedPtr<FJsonObject> JsonObject;
		    TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(RPC.Payload);
		    if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
		    {
			    UE_LOG(LogJson, Warning, TEXT("JsonObjectStringToUStruct - Unable to parse json=[%s]"), *JsonString);
			    Result = ECallbackResponseLeaderboardEntries::ErrorUnspecified;
		    }
		    else
		    {
			    // Need to manually parse json for named dictionary values
			    for (const FString& ID : IN_LeaderboardIDs)
			    {
				    const TSharedPtr<FJsonObject> pItem = JsonObject->GetObjectField(ID);
				    if (!pItem || pItem->Values.Num() == 0) continue;
				    // const TSharedRef<FJsonObject> ItemRef = pItem.ToSharedRef();

				    FString LeaderboardJsonString = TEXT("");
				    if (!JsonObjectToString(pItem, LeaderboardJsonString)) continue;

				    FLeaderboardEntryJson LeaderboardEntryJson;
				    if (FJsonObjectConverter::JsonObjectStringToUStruct(LeaderboardJsonString, &LeaderboardEntryJson, 0, 0))
				    {
					    LeaderboardEntries.Entries.Add(ID, LeaderboardEntryJson.ToLeaderboardEntry());
				    }
			    }
		    }

		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *RPC.Id, *RPC.Payload);
		    IN_Callback.ExecuteIfBound(Result, LeaderboardEntries);
	    });
	FOnErrorDelegate OnError = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& Error)
	    {
		    ECallbackResponseLeaderboardEntries Result = ECallbackResponseLeaderboardEntries::ErrorUnspecified;
		    FLeaderboardEntries LeaderboardEntries;

		    UE_LOG(LogNakamaManager, Log, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(Result, LeaderboardEntries);
	    });

	NakamaClient->RPC(UserSession, Function, Payload, OnSuccess, OnError);
}

void UNakamaManager::GetLeaderboardEntriesAroundPlayer(bool IN_bGlobal,
                                                       const FString& IN_LeaderboardID,
                                                       int32 IN_Entries,
                                                       const TArray<FName>& IN_FriendIDs,
                                                       int32 IN_IncludeFirst,
                                                       int32 IN_IncludeLast,
                                                       bool IN_bSocial,
                                                       bool IN_bInverseSocial,
                                                       const TArray<FName>& IN_TeamIDs,
                                                       const TArray<FName>& IN_TeamTypes,
                                                       const FOnGetLeaderboardEntriesAroundPlayerCompleteDelegate& IN_Callback)
{
	// #todo
	const FString Function = TEXT("AroundMeLeaderboardRequest");
	const FString Payload = FString::Printf(TEXT("{\"leaderboardShortCode\" : \"%s\", \"dontErrorOnNotSocial\" : false, \"entryCount\" : %d, \"includeFirst\" : %d, \"includeLast\" : %d, \"social\" : %s, \"inverseSocial\": %s }"),
	                                        *IN_LeaderboardID,
	                                        IN_Entries,
	                                        IN_IncludeFirst,
	                                        IN_IncludeLast,
	                                        IN_bSocial ? TEXT("true") : TEXT("false"),
	                                        IN_bInverseSocial ? TEXT("true") : TEXT("false"));

	FOnRPCDelegate OnSuccess = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& IN_RPC)
	    {
		    FLeaderboardData LeaderboardData;
		    ECallbackResponseLeaderboardEntriesAroundPlayer Result = ECallbackResponseLeaderboardEntriesAroundPlayer::Success;

		    FNakamaRPCErrorResponse ErrorResponse;
		    if (FJsonObjectConverter::JsonObjectStringToUStruct(IN_RPC.Payload, &ErrorResponse, 0, 0))
		    {
			    if (ErrorResponse.error.leaderboardShortCode.IsEmpty())
			    {
				    // Success!

				    FLeaderboardEntryArrayJson Json;
				    if (FJsonObjectConverter::JsonObjectStringToUStruct(IN_RPC.Payload, &Json, 0, 0))
				    {
					    Result = ECallbackResponseLeaderboardEntriesAroundPlayer::Success;
					    LeaderboardData = Json.ToLeaderboardData();
				    }
			    }
			    else
			    {
				    if (ErrorResponse.error.leaderboardShortCode == TEXT("NO_ENTRY"))
				    {
					    Result = ECallbackResponseLeaderboardEntriesAroundPlayer::NoEntry;
				    }
				    else if (ErrorResponse.error.leaderboardShortCode == TEXT("INVALID"))
				    {
					    Result = ECallbackResponseLeaderboardEntriesAroundPlayer::Empty;
				    }
				    else
				    {
					    ensureAlways(false);
					    Result = ECallbackResponseLeaderboardEntriesAroundPlayer::ErrorUnspecified;
				    }
			    }
		    }

		    //#todo: FIll out leaderboard data

		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *IN_RPC.Id, *IN_RPC.Payload);
		    IN_Callback.ExecuteIfBound(Result, LeaderboardData);
	    });
	FOnErrorDelegate OnError = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& Error)
	    {
		    ECallbackResponseLeaderboardEntriesAroundPlayer Result = ECallbackResponseLeaderboardEntriesAroundPlayer::ErrorUnspecified;

		    UE_LOG(LogNakamaManager, Log, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(Result, FLeaderboardData());
	    });

	NakamaClient->RPC(UserSession, Function, Payload, OnSuccess, OnError);
}

void UNakamaManager::GetLeaderboardEntry(bool IN_bGlobal, const FString& IN_LeaderboardID, const FName& IN_PlayerID, const FOnGetLeaderboardEntryCompleteDelegate& IN_Callback)
{
	FOnGetLeaderboardEntriesCompleteDelegate OnComplete = FOnGetLeaderboardEntriesCompleteDelegate::CreateLambda(
	    [=](ECallbackResponseLeaderboardEntries IN_CallbackResponse, const FLeaderboardEntries& IN_Entries)
	    {
		    if (ECallbackResponseLeaderboardEntries::Success == IN_CallbackResponse)
		    {
			    // If the request came back successfully, the player either has an entry in the leaderboard specified, or doesn't. Interpret/Handle both.
			    if (IN_Entries.Entries.Contains(IN_LeaderboardID))
			    {
				    IN_Callback.ExecuteIfBound(ECallbackResponseLeaderboardEntry::Success, IN_Entries.Entries[IN_LeaderboardID]);
			    }
			    else
			    {
				    IN_Callback.ExecuteIfBound(ECallbackResponseLeaderboardEntry::NoEntry, FLeaderboardEntry());
			    }
		    }
		    else
		    {
			    switch (IN_CallbackResponse)
			    {
				    case ECallbackResponseLeaderboardEntries::ErrorTimeout:
					    IN_Callback.ExecuteIfBound(ECallbackResponseLeaderboardEntry::ErrorTimeout, FLeaderboardEntry());
					    break;
				    case ECallbackResponseLeaderboardEntries::ErrorUnspecified:
					    IN_Callback.ExecuteIfBound(ECallbackResponseLeaderboardEntry::ErrorUnspecified, FLeaderboardEntry());
					    break;
				    default:
					    break;
			    }
		    }
	    });

	GetLeaderboardEntries(IN_bGlobal, {IN_LeaderboardID}, IN_PlayerID, false, false, {}, OnComplete);
}

void UNakamaManager::GetLeaderboardNewHighScores(TArray<FLeaderboardNewHighScore>& OUT_NewHighScores)
{
	// todo
	ensure(0 == OUT_NewHighScores.Num());
	OUT_NewHighScores.Append(LeaderboardNewHighScoresArray);
}

void UNakamaManager::GetLeaderboardGlobalRankChanges(TArray<FLeaderboardRankChanged>& OUT_GlobalRankChanges)
{
	// todo
	ensure(0 == OUT_GlobalRankChanges.Num());
	OUT_GlobalRankChanges.Append(LeaderboardGlobalRankChanges);
}

void UNakamaManager::GetLeaderboardSocialRankChanges(TArray<FLeaderboardRankChanged>& OUT_SocialRankChanges)
{
	// todo
	ensure(0 == OUT_SocialRankChanges.Num());
	OUT_SocialRankChanges.Append(LeaderboardSocialRankChanges);
}

FOnLeaderboardNewHighScoreEvent& UNakamaManager::GetOnLeaderboardNewHighScoreEvent() { return Event_OnLeaderboardNewHighScore; }
FOnLeaderboardRankChangedEvent& UNakamaManager::GetOnLeaderboardGlobalRankChangedEvent() { return Event_OnLeaderboardGlobalRankChangedEvent; }
FOnLeaderboardRankChangedEvent& UNakamaManager::GetOnLeaderboardSocialRankChangedEvent() { return Event_OnLeaderboardSocialRankChangedEvent; }

void UNakamaManager::OnWriteToStorageComplete(const FNakamaStoreObjectAcks& StorageObjectsAcks)
{
	//
	UE_LOG(LogNakamaManager, Log, TEXT("Successfully stored objects: %i"), StorageObjectsAcks.StorageObjects.Num());
}

void UNakamaManager::OnWriteToStorageError(const FNakamaError& IN_Error)
{
	//
	UE_LOG(LogNakamaManager, Log, TEXT("Failed to write to storage. Code: %s %s"), *UEnum::GetDisplayValueAsText(IN_Error.Code).ToString(), *IN_Error.Message);
}

const FMaintenanceStatus& UNakamaManager::GetCachedMaintenanceStatus() const { return CachedMaintenanceStatus; }
FOnMaintenanceStartedEvent& UNakamaManager::GetOnMaintenanceStartedEvent() { return Event_OnMaintenanceStarted; }
FOnMaintenanceEndedEvent& UNakamaManager::GetOnMaintenanceEndedEvent() { return Event_OnMaintenanceEnded; }

void UNakamaManager::WriteToStorage(const FString& Collection, const FString& Key, const FString& Value)
{
	FNakamaStoreObjectWrite Data;
	Data.Collection = Collection;
	Data.Key = Key;
	Data.Value = Value;
	Data.PermissionRead = ENakamaStoragePermissionRead::NO_READ;
	Data.PermissionWrite = ENakamaStoragePermissionWrite::OWNER_WRITE;

	FOnStorageObjectAcks OnComplete;
	OnComplete.AddDynamic(this, &UNakamaManager::OnWriteToStorageComplete);

	FOnError OnError;
	OnError.AddDynamic(this, &UNakamaManager::OnWriteToStorageError);

	NakamaClient->WriteStorageObjects(UserSession, {Data}, OnComplete, OnError);
}*/

void UNakamaManager::Authenticate()
{
	FOnAuthUpdate AuthenticationSuccessDelegate;
	AuthenticationSuccessDelegate.AddDynamic(this, &UNakamaManager::OnAuthenticationSuccessIDRetrieval);

	FOnError AuthenticationErrorDelegate;
	AuthenticationErrorDelegate.AddDynamic(this, &UNakamaManager::OnAuthenticationError);

	NakamaClient->AuthenticateDevice("ID-Retrieval", "ID-Retrieval", true, {}, AuthenticationSuccessDelegate, AuthenticationErrorDelegate);
}

void UNakamaManager::AuthenticateUsingGamesparksID(FString IN_GamesparksID)
{
	FString SteamID;
	FString SteamAuthToken;
	FString SteamNickName;
	bool bHasSteam = GetSteamData(SteamID, SteamAuthToken, SteamNickName);
	if (!bHasSteam)
	{
		ensure(false);
		return;
	}

	FOnAuthUpdate AuthenticationSuccessDelegate;
	AuthenticationSuccessDelegate.AddDynamic(this, &UNakamaManager::OnAuthenticationSuccess);

	FOnError AuthenticationErrorDelegate;
	AuthenticationErrorDelegate.AddDynamic(this, &UNakamaManager::OnAuthenticationError);

	NakamaClient->AuthenticateCustom(IN_GamesparksID, "", true, {{"token", SteamAuthToken}}, AuthenticationSuccessDelegate, AuthenticationErrorDelegate);
}

void UNakamaManager::AuthenticateUsingSteam()
{
	FString SteamID;
	FString SteamAuthToken;
	FString SteamNickName;
	// FString UserName;
	bool bCreateAccount = true;
	TMap<FString, FString> Vars;

	bool bHasSteam = GetSteamData(SteamID, SteamAuthToken, SteamNickName);

	if (!bHasSteam)
	{
		//#todo: Handle nicely
		InitialisationStatus = ENakamaInitialisationStatus::ErrorFailedToAuthenticate;
		ensure(false);
		return;
	}

	UserDisplayName = SteamNickName;    // Set display name to Steam name by default.

	// Setup Delegates of same type and bind them to local functions
	FOnAuthUpdate AuthenticationSuccessDelegate;
	AuthenticationSuccessDelegate.AddDynamic(this, &UNakamaManager::OnAuthenticationSuccess);

	FOnError AuthenticationErrorDelegate;
	AuthenticationErrorDelegate.AddDynamic(this, &UNakamaManager::OnSteamAuthenticationError);

	ensure(NakamaClient);

	if (NakamaClient)
	{
		UE_LOG(LogNakamaManager, Verbose, TEXT("Authenticating with SteamID %s and Token %s"), *SteamID, *SteamAuthToken);

		NakamaClient->AuthenticateSteam(SteamAuthToken, SteamID, bCreateAccount, Vars, AuthenticationSuccessDelegate, AuthenticationErrorDelegate);
	}
}

void UNakamaManager::AuthenticateAnonymous()
{
	// throw std::logic_error("The method or operation is not implemented.");

	FOnAuthUpdate AuthenticationSuccessDelegate;
	AuthenticationSuccessDelegate.AddDynamic(this, &UNakamaManager::OnAuthenticationSuccess);

	FOnError AuthenticationErrorDelegate;
	AuthenticationErrorDelegate.AddDynamic(this, &UNakamaManager::OnAuthenticationError);

	FString DeviceID = FGuid::NewGuid().ToString();
	NakamaClient->AuthenticateDevice(DeviceID, UserName, true, {}, AuthenticationSuccessDelegate, AuthenticationErrorDelegate);
}

void UNakamaManager::AuthenticateServer()
{
	FOnAuthUpdate AuthenticationSuccessDelegate;
	AuthenticationSuccessDelegate.AddDynamic(this, &UNakamaManager::OnAuthenticationSuccess);

	FOnError AuthenticationErrorDelegate;
	AuthenticationErrorDelegate.AddDynamic(this, &UNakamaManager::OnAuthenticationError);

	const FString DeviceID = FGuid::NewGuid().ToString();
	NakamaClient->AuthenticateDevice(DeviceID, "", true, {}, AuthenticationSuccessDelegate, AuthenticationErrorDelegate);
}

void UNakamaManager::AuthenticateCustom()
{
	FOnAuthUpdate AuthenticationSuccessDelegate;
	AuthenticationSuccessDelegate.AddDynamic(this, &UNakamaManager::OnAuthenticationSuccess);

	FOnError AuthenticationErrorDelegate;
	AuthenticationErrorDelegate.AddDynamic(this, &UNakamaManager::OnAuthenticationError);

	UserDisplayName = UserName;

	NakamaClient->AuthenticateCustom(UserID, UserName, true, {}, AuthenticationSuccessDelegate, AuthenticationErrorDelegate);
}

void UNakamaManager::OnAuthenticationSuccessIDRetrieval(UNakamaSession* IN_LoginData)
{
	UserSession = IN_LoginData;

	FOnRPC RPCSuccessDelegate;
	RPCSuccessDelegate.AddDynamic(this, &UNakamaManager::OnGamesparksIDRetrieved);

	FOnError RPCErrorDelegate;
	RPCErrorDelegate.AddDynamic(this, &UNakamaManager::OnRPCError);

	FString SteamID;
	FString SteamAuthToken;
	FString SteamNickName;
	bool bValidSteam = GetSteamData(SteamID, SteamAuthToken, SteamNickName);

	if (!bValidSteam)
	{
		ensure(false);
		return;
	}

	FString FunctionName = TEXT("_getId");
	FString FunctionPayload = FString::Printf(TEXT("{\"steamId\":\"%s\"}"), *SteamID);
	NakamaClient->RPC(UserSession, FunctionName, FunctionPayload, RPCSuccessDelegate, RPCErrorDelegate);
}

void UNakamaManager::OnAuthenticationSuccess(UNakamaSession* IN_LoginData)
{
	//TODO: IMPLEMENT
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Authenticated As %s"), *IN_LoginData->SessionData.Username));

	UE_LOG(LogNakamaManager, Log, TEXT("Successfully authenticated as %s"), *IN_LoginData->SessionData.UserId);

	UserSession = IN_LoginData;
	NakamaUserID = FName(*IN_LoginData->SessionData.UserId);
	//MongoDBUserID = IInterfaceBackendService::ToMongoDBUserID(NakamaUserID);

	// Realtime client (grpc)
	static bool s_StartRealtimeClient = true;

	if (s_StartRealtimeClient)
	{

		// Setup Delegates of same type and bind them to local functions
		FOnRealtimeClientConnected ConnectionSuccessDelegate;
		ConnectionSuccessDelegate.AddDynamic(this, &UNakamaManager::OnRealtimeClientConnectSuccess);

		FOnRealtimeClientError ConnectionErrorDelegate;
		ConnectionErrorDelegate.AddDynamic(this, &UNakamaManager::OnRealtimeClientConnectError);

		// This is our realtime client (socket) ready to use
		NakamaRealtimeClient = NakamaClient->SetupRealtimeClient(UserSession, true, 7350, ENakamaRealtimeClientProtocol::Protobuf, 0.05f, "");

		// Remember to Connect
		NakamaRealtimeClient->Connect(ConnectionSuccessDelegate, ConnectionErrorDelegate);
	}

	/*GetMaintenanceStatus(FOnGetMaintenanceStatusCompleteDelegate::CreateLambda(
	    [this](bool IN_bSucceeded, const FMaintenanceStatus& IN_Status)
	    {
		    // ensure(IN_bSucceeded);
		    // if (IN_bSucceeded)
		    {
			    CachedMaintenanceStatus = IN_Status;

			    if (!CachedMaintenanceStatus.bDownNow)
			    {
					// Update user display name in cloud if we have one locally 

					if (!UserDisplayName.IsEmpty())
					{
						FOnUpdateAccount OnSuccess;
						OnSuccess.AddDynamic(this, &UNakamaManager::OnAccountUpdateSucceeded);
						FOnError OnFail;
						OnFail.AddDynamic(this, &UNakamaManager::OnAccountUpdateFailed);

					
						NakamaClient->UpdateAccountDisplayName(UserSession, UserSession->SessionData.Username, UserDisplayName, OnSuccess, OnFail);
					}

					//Now attempt to set user location data if we can get it from Nakama metadata

					auto OnSuccess = [this](const NAccount& account) {  
						FNakamaUserMetaData UserMetaData;
						bool bSuccess = FJsonObjectConverter::JsonObjectStringToUStruct(FString(account.user.metadata.c_str()), &UserMetaData, 0, 0);
						ensure(bSuccess);
						if (bSuccess)
						{
							SetAuthenticatedUserCountry(UserMetaData.location.country);
							SetAuthenticatedUserCity(UserMetaData.location.city);
						}

						InitialisationStatus = ENakamaInitialisationStatus::Initialised;
					};

					auto OnFail = [this](const NError& error) { InitialisationStatus = ENakamaInitialisationStatus::ErrorGetAccountDetailsFailed; };

					NakamaClient->Client->getAccount(UserSession->UserSession, OnSuccess, OnFail);  
			    }
			    else
			    {
				    InitialisationStatus = ENakamaInitialisationStatus::DownForMainenance;
			    }
		    }
		    // else
		    //{
		    //	InitialisationStatus = ENakamaInitialisationStatus::ErrorGetMaintenanceStatusFailed;
		    //}
	    }));*/

	InitialisationStatus = ENakamaInitialisationStatus::Initialised;
}

void UNakamaManager::OnAuthenticationError(const FNakamaError& IN_Error)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("Authentication failed. Code: %s %s"), *UEnum::GetDisplayValueAsText(IN_Error.Code).ToString(), *IN_Error.Message));
	UE_LOG(LogNakamaManager, Log, TEXT("Failed to authenticate. Code: %s %s"), *UEnum::GetDisplayValueAsText(IN_Error.Code).ToString(), *IN_Error.Message);
}

void UNakamaManager::OnSteamAuthenticationError(const FNakamaError& IN_Error)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("Authentication failed. Code: %s %s"), *UEnum::GetDisplayValueAsText(IN_Error.Code).ToString(), *IN_Error.Message));
	UE_LOG(LogNakamaManager, Log, TEXT("Failed to authenticate with Steam. Code: %s %s"), *UEnum::GetDisplayValueAsText(IN_Error.Code).ToString(), *IN_Error.Message);

	if (--SteamAuthenticationRetries <= 0)
	{
		InitialisationStatus = ENakamaInitialisationStatus::ErrorFailedToAuthenticate;
		return;
	}

	UE_LOG(LogNakamaManager, Log, TEXT("Retrying Steam authentication..."));
	FTimerHandle UnusedHandle;
	GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &UNakamaManager::AuthenticateUsingSteam, 1.5f, false);
}

void UNakamaManager::OnAccountUpdateSucceeded()
{
	UE_LOG(LogNakamaManager, Log, TEXT("Account update succeeded."));
}

void UNakamaManager::OnAccountUpdateFailed(const FNakamaError& IN_Error)
{
	UE_LOG(LogNakamaManager, Log, TEXT("Account Update Failed: %s - %s"), *UEnum::GetDisplayValueAsText(IN_Error.Code).ToString(), *IN_Error.Message);
}

void UNakamaManager::OnRealtimeClientConnectSuccess()
{
	ensure(NakamaRealtimeClient);
	if (NakamaRealtimeClient)
	{
		FOnReceivedNotification NotificationReceivedDelegate;
		//NotificationReceivedDelegate.AddDynamic(this, &UNakamaManager::OnReceivedNotification); //TODO: IMPLEMENT

		//NakamaRealtimeClient->NotificationReceived = NotificationReceivedDelegate;
		//NakamaRealtimeClient->SetListenerNotificationsCallback();

		//NakamaRealtimeClient->SetListenerAllCallbacks();
	}
}

void UNakamaManager::OnRealtimeClientConnectError() {}

const int32 NEW_HIGH_SCORE = 1;

/*void UNakamaManager::OnReceivedNotification(const FNakamaNotificationList& NotificationList)
{
	for (const FNakamaNotification& Notification : NotificationList.Notifications)
	{
		UE_LOG(LogNakamaManager, Verbose, TEXT("Received notification. Code: %d Id: %s Subject: %s Content: %s"), Notification.Code, *Notification.Id, *Notification.Subject, *Notification.Content);

		switch (Notification.Code)
		{
			case NEW_HIGH_SCORE:
			{
				FLeaderboardNotification json;
				bool bSuccess = FJsonObjectConverter::JsonObjectStringToUStruct(Notification.Content, &json, 0, 0);
				ensure(bSuccess);

				if (bSuccess)
				{
					FLeaderboardNewHighScore LeaderboardNewHighScore;
					LeaderboardNewHighScore.LeaderboardName = json.leaderboardName;
					LeaderboardNewHighScore.RankDetails = json.rankDetails.ToLeaderboardRankDetails();

					LeaderboardNewHighScoresArray.Add(LeaderboardNewHighScore);
					Event_OnLeaderboardNewHighScore.Broadcast(LeaderboardNewHighScore);
				}
			}
			break;

			default:
				break;
		}
	}
}*/

void UNakamaManager::OnRPCSuccess(const FNakamaRPC& IN_RPC)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("RPC succeeded. ID: %s Payload: %s"), *IN_RPC.Id, *IN_RPC.Payload));
}

void UNakamaManager::OnRPCError(const FNakamaError& IN_Error)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("RPC failed: %s"), *IN_Error.Message));
}

void UNakamaManager::OnLogEventSuccess(const FNakamaRPC& IN_RPC)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("RPC succeeded. ID: %s Payload: %s"), *IN_RPC.Id, *IN_RPC.Payload));
}

void UNakamaManager::OnGamesparksIDRetrieved(const FNakamaRPC& IN_RPC)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("RPC succeeded. ID: %s Payload: %s"), *IN_RPC.Id, *IN_RPC.Payload));

	FNakamaID NakamaID;

	bool bSucces = FJsonObjectConverter::JsonObjectStringToUStruct(IN_RPC.Payload, &NakamaID, 0, 0);
	ensure(bSucces);

	if (bSucces && !NakamaID.id.IsEmpty())
	{
		AuthenticateUsingGamesparksID(NakamaID.id);
	}
	else
	{
		AuthenticateUsingSteam();
	}
}

bool UNakamaManager::GetSteamData(FString& OUT_ID, FString& OUT_Token, FString& OUT_NickName) const
{
#if PLATFORM_WINDOWS
	IOnlineIdentityPtr Result = nullptr;

	auto test = STEAM_SUBSYSTEM;
	IOnlineSubsystem* pOnlineSubsystem = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
	ensure(pOnlineSubsystem);
	if (!pOnlineSubsystem)
	{
		UE_LOG(LogNakamaManager, Log, TEXT("Failed to authenticate with Steam: No subsystem loaded."));
		return false;
	}

	FOnlineSubsystemSteam* pSteamOnlineSubsystem = static_cast<FOnlineSubsystemSteam*>(pOnlineSubsystem);
	ensure(pSteamOnlineSubsystem);
	if (!pSteamOnlineSubsystem)
	{
		UE_LOG(LogNakamaManager, Log, TEXT("Failed to authenticate with Steam: No Steam subsystem loaded."));
		return false;
	}

	ensure(pSteamOnlineSubsystem->IsSteamClientAvailable());
	if (!pSteamOnlineSubsystem->IsSteamClientAvailable())
	{
		// 		UE_LOG(LogGameSparksTasks, Log, TEXT("UTaskGameSparksAuthenticateWithSteam::Authenticate - The online subsystem was created but the steamworks init failed"));
		// 		return ESteamAuthenticationStatus::SteamworksClientInitialisationFailed;
		UE_LOG(LogNakamaManager, Log, TEXT("Failed to authenticate with Steam: Steam client unavailable."));
		return false;
	}

	Result = pSteamOnlineSubsystem->GetIdentityInterface();
	ensure(Result.IsValid());

	if (!Result.IsValid())
	{
		UE_LOG(LogNakamaManager, Log, TEXT("Failed to authenticate with Steam: Failed to get a valid identity."));
		return false;
	}

	FUniqueNetIdPtr pUniqueNetId = Result->GetUniquePlayerId(0);
	ensure(pUniqueNetId);
	if (!pUniqueNetId) return false;

	OUT_ID = pUniqueNetId->ToString();
	OUT_Token = Result->GetAuthToken(0);
	OUT_NickName = Result->GetPlayerNickname(*pUniqueNetId);
#else
	return false;
#endif    // PLATFORM_WINDOWS
	return true;
}

/*void UNakamaManager::GetMaintenanceStatus(const FOnGetMaintenanceStatusCompleteDelegate& IN_Callback)
{
	const FString Function = TEXT("GET_MAINTENANCE_STATUS");
	const FString Payload = TEXT("");

	FOnRPCDelegate RPCSuccessDelegate = FOnRPCDelegate::CreateLambda(
	    [=](const FNakamaRPC& IN_RPC)
	    {
		    FMaintenanceStatus Status;
		    bool bSuccess = FJsonObjectConverter::JsonObjectStringToUStruct(GetJsonStringFromRPC(IN_RPC), &Status, 0, 0);
		    UE_LOG(LogNakamaManager, Verbose, TEXT("LogEvent RPC called: %s(%s) - result: %s %s"), *Function, *Payload, *IN_RPC.Id, *IN_RPC.Payload);
		    IN_Callback.ExecuteIfBound(bSuccess, Status);
	    });
	FOnErrorDelegate RPCErrorDelegate = FOnErrorDelegate::CreateLambda(
	    [=](const FNakamaError& IN_Error)
	    {
		    FMaintenanceStatus Status;
		    UE_LOG(LogNakamaManager, Log, TEXT("LogEvent Error called: %s(%s) - result: %s"), *Function, *Payload, *UEnum::GetDisplayValueAsText(IN_Error.Code).ToString());
		    IN_Callback.ExecuteIfBound(false, Status);
	    });

	NakamaClient->RPC(UserSession, Function, Payload, RPCSuccessDelegate, RPCErrorDelegate);
}

/*

void UNakamaManager::OnNewHighScore(FGSNewHighScoreMessage IN_GSNewHighScoreMessage)
{
    FLeaderboardNewHighScore LeaderboardNewHighScore;

    ensure(IN_GSNewHighScoreMessage.HasLeaderboardName);
    LeaderboardNewHighScore.LeaderboardName = IN_GSNewHighScoreMessage.LeaderboardName;

    ensure(IN_GSNewHighScoreMessage.HasRankDetails);
    GSLeaderboardRankDetailsToLeaderboardRankDetails(IN_GSNewHighScoreMessage.RankDetails, LeaderboardNewHighScore.RankDetails);

    LeaderboardNewHighScoresArray.Add(LeaderboardNewHighScore);

    Event_OnLeaderboardNewHighScore.Broadcast(LeaderboardNewHighScore);
}

void UGameSparksManager::OnGlobalRankChanged(FGSGlobalRankChangedMessage IN_GSGlobalRankChangedMessage)
{
    FLeaderboardRankChanged LeaderboardRankChanged;

    ensure(IN_GSGlobalRankChangedMessage.HasLeaderboardName);
    LeaderboardRankChanged.LeaderboardName = IN_GSGlobalRankChangedMessage.LeaderboardName;

    ensure(IN_GSGlobalRankChangedMessage.HasLeaderboardShortCode);
    LeaderboardRankChanged.LeaderboardShortCode = IN_GSGlobalRankChangedMessage.LeaderboardShortCode;

    ensure(IN_GSGlobalRankChangedMessage.HasYou);
    GSLeaderboardDataToLeaderboardEntry(IN_GSGlobalRankChangedMessage.You, LeaderboardRankChanged.YourNewRank);

    ensure(IN_GSGlobalRankChangedMessage.HasThem);
    GSLeaderboardDataToLeaderboardEntry(IN_GSGlobalRankChangedMessage.Them, LeaderboardRankChanged.TheirNewRank);

    LeaderboardGlobalRankChanges.Add(LeaderboardRankChanged);

    Event_OnLeaderboardGlobalRankChangedEvent.Broadcast(LeaderboardRankChanged);
}

void UGameSparksManager::OnSocialRankChanged(FGSSocialRankChangedMessage IN_GSSocialRankChangedMessage)
{
    FLeaderboardRankChanged LeaderboardRankChanged;

    ensure(IN_GSSocialRankChangedMessage.HasLeaderboardName);
    LeaderboardRankChanged.LeaderboardName = IN_GSSocialRankChangedMessage.LeaderboardName;

    ensure(IN_GSSocialRankChangedMessage.HasLeaderboardShortCode);
    LeaderboardRankChanged.LeaderboardShortCode = IN_GSSocialRankChangedMessage.LeaderboardShortCode;

    ensure(IN_GSSocialRankChangedMessage.HasYou);
    GSLeaderboardDataToLeaderboardEntry(IN_GSSocialRankChangedMessage.You, LeaderboardRankChanged.YourNewRank);

    ensure(IN_GSSocialRankChangedMessage.HasThem);
    GSLeaderboardDataToLeaderboardEntry(IN_GSSocialRankChangedMessage.Them, LeaderboardRankChanged.TheirNewRank);

    LeaderboardSocialRankChanges.Add(LeaderboardRankChanged);

    Event_OnLeaderboardSocialRankChangedEvent.Broadcast(LeaderboardRankChanged);
}

void UGameSparksManager::OnAchievementEarned(FGSAchievementEarnedMessage IN_GSAchievementEarnedMessage)
{
    FAchievementEarned AchievementEarned;

    ensure(IN_GSAchievementEarnedMessage.HasAchievementShortCode);
    AchievementEarned.AchievementShortCode = *IN_GSAchievementEarnedMessage.AchievementShortCode;

    // Not sure if achievements will always have XP associated
    ensure(IN_GSAchievementEarnedMessage.HasCurrencyAwards);
    ensure(IN_GSAchievementEarnedMessage.CurrencyAwards->HasNumber("XP"));
    if (IN_GSAchievementEarnedMessage.HasCurrencyAwards && IN_GSAchievementEarnedMessage.CurrencyAwards->HasNumber("XP"))
    {
        AchievementEarned.XP = IN_GSAchievementEarnedMessage.CurrencyAwards->GetNumber("XP");
    }

    AchievementsEarnedArray.Add(AchievementEarned);

    Event_OnAchievementEarned.Broadcast(AchievementEarned);
}

void UGameSparksManager::OnScriptMessage(FGSScriptMessage IN_GSScriptMessage)
{
    static const FString XpLevelUpMessageShortCode = "XP_LEVEL_UP_MESSAGE";
    static const FString MaintenanceStartMessageShortCode = "MAINTENANCE_START";
    static const FString MaintenanceEndMessageShortCode = "MAINTENANCE_END";

    if (IN_GSScriptMessage.ExtCode == XpLevelUpMessageShortCode)
    {
        FXpLevelUp XpLevelUp;

        ensure(IN_GSScriptMessage.HasData);
        ensure(IN_GSScriptMessage.Data->HasNumber("newLevel"));
        XpLevelUp.NewLevel = IN_GSScriptMessage.Data->GetNumber("newLevel");

        OnXpLevelUp(XpLevelUp);
    }
    else if (IN_GSScriptMessage.ExtCode == MaintenanceStartMessageShortCode)
    {
        CachedMaintenanceStatus.Reset();

        ExtractMaintenanceStatusFromGSScriptMessage(IN_GSScriptMessage, CachedMaintenanceStatus);
        ensure(CachedMaintenanceStatus.bDownNow);
        ensure(!CachedMaintenanceStatus.Message.IsEmpty());

        Event_OnMaintenanceStarted.Broadcast();
    }
    else if (IN_GSScriptMessage.ExtCode == MaintenanceEndMessageShortCode)
    {
        ExtractMaintenanceStatusFromGSScriptMessage(IN_GSScriptMessage, CachedMaintenanceStatus);
        ensure(!CachedMaintenanceStatus.bDownNow);

        Event_OnMaintenanceEnded.Broadcast();
    }
}
*/

/*FLeaderboardRankDetails FLeaderboardRankDetailsJson::ToLeaderboardRankDetails() const
{

	FLeaderboardRankDetails Result;
	Result.GlobalCount = this->globalCount;
	Result.GlobalFrom = this->globalFrom;
	Result.GlobalFromPercent = this->globalFromPercent;
	Result.GlobalTo = this->globalTo;
	Result.GlobalToPercent = this->globalToPercent;

	Result.SocialCount = this->socialCount;
	Result.SocialFrom = this->socialFrom;
	Result.SocialFromPercent = this->socialFromPercent;
	Result.SocialTo = this->socialTo;
	Result.SocialToPercent = this->socialToPercent;

	// Result.FriendsPassed = this->topNPassed;

	return Result;
}*/

//PRAGMA_ENABLE_OPTIMIZATION
