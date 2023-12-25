#include "NakamaManager.h"

#include "JsonObjectConverter.h"
#include "NakamaClient.h"
#include <Templates/SharedPointer.h>
#include "ShopManagerClient.h"
#include "UserManagerClient.h"

#if PLATFORM_WINDOWS
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemSteam.h"
#endif

DEFINE_LOG_CATEGORY(LogNakamaManager);

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
	//return "";
}

UNakamaManager::UNakamaManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsConnected(false)
	, AuthenticationRetriesMax(5)
	, bUsePreviewServer(true)
	, bVCChangeAllowed(false)
	, UserManagerClient(CreateDefaultSubobject<UUserManagerClient>("UserManager"))
	, ShopManagerClient(CreateDefaultSubobject<UShopManagerClient>("ShopManager"))
	//, CachedMaintenanceStatus(FMaintenanceStatus())
	, PreviewServerKey(TEXT("1hP9lwynAatdvFC5"))
	, ProductionServerKey(TEXT("vrTxrvK441T3O6bk"))
	, PreviewHost(TEXT("kartkraft.as-neast3-a.nakamacloud.io"))
	, ProductionHost(TEXT("kartkraft-prod.us-east1-c.nakamacloud.io"))
	, InitialisationStatus(ENakamaInitialisationStatus::Uninitialised)
	, bIsDedicatedServer(false)
{
	LoadConfig();

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
			UserDisplayName = TEXT("DedicatedServerUserName1");
			InitialisationStatus = ENakamaInitialisationStatus::Initialised;    //#hack no server auth for now. Having trouble getting it to work on Linux.
			// AuthenticateServer();
			break;
		case EAuthenticationMethod::Steam:
			AuthenticateUsingSteam();
			break;
		default:
			ensure(false);
			break;
	};
}

void UNakamaManager::AuthenticateUsingSteam()
{
	FString SteamAuthToken;
	FString SteamNickName;
	// FString UserName;
	bool bCreateAccount = true;
	TMap<FString, FString> Vars;

	bool bHasSteam = GetSteamData(SteamID, SteamAuthToken, SteamNickName);

	if (!bHasSteam)
	{
		InitialisationStatus = ENakamaInitialisationStatus::ErrorFailedToAuthenticate;
		//ensure(false);
		return;
	}

	UserDisplayName = SteamNickName; // Set display name to Steam name by default.

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

void UNakamaManager::OnAuthenticationSuccess(UNakamaSession* IN_LoginData)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Authenticated As %s"), *IN_LoginData->SessionData.Username));
	UE_LOG(LogNakamaManager, Log, TEXT("Successfully authenticated as %s"), *IN_LoginData->SessionData.UserId);

	UserSession = IN_LoginData;
	NakamaUserID = FName(*IN_LoginData->SessionData.UserId);

	UserManagerClient->Initialise(this);
	UserManagerClient->RequestHeroData();

	ShopManagerClient->Initialise(NakamaClient, UserSession, this);

	InitialisationStatus = ENakamaInitialisationStatus::Initialised;
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

void UNakamaManager::OnRPCSuccess(const FNakamaRPC& IN_RPC)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("RPC succeeded. ID: %s Payload: %s"), *IN_RPC.Id, *IN_RPC.Payload));
}

void UNakamaManager::OnRPCError(const FNakamaError& IN_Error)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("RPC failed: %s"), *IN_Error.Message));
}

bool UNakamaManager::GetSteamData(FString& OUT_ID, FString& OUT_Token, FString& OUT_NickName) const
{
#if PLATFORM_WINDOWS
	IOnlineIdentityPtr Result = nullptr;

	IOnlineSubsystem* pOnlineSubsystem = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
	//ensure(pOnlineSubsystem);
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
