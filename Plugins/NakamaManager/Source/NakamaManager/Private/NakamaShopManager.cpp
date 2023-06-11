#include "NakamaShopManager.h"

#include <steam/isteamuser.h>

#include "JsonObjectConverter.h"
#include "NakamaClient.h"
#include "NakamaManager.h"
#include "NakamaSession.h"

void UNakamaShopManager::Initialise(UNakamaClient* IN_NakamaClient, UNakamaSession* IN_UserSession, UNakamaManager* IN_NakamaManager)
{
	if (!IsValid(IN_NakamaClient) || !IsValid(IN_UserSession))
	{
		check(false);
		return;
	}

	NakamaClient = IN_NakamaClient;
	UserSession = IN_UserSession;
	NakamaManager = IN_NakamaManager;

	FOnRPC OnReceivedAllBundlesSuccess;
	OnReceivedAllBundlesSuccess.AddDynamic(this, &UNakamaShopManager::OnReceivedAllBundles);
	NakamaClient->RPC(UserSession, "get_all_bundles", "", OnReceivedAllBundlesSuccess, {});
}

void UNakamaShopManager::BuyBundle(const uint32 BundleID)
{
	//TODO: Branch defines for different platforms

# ifdef USING_STEAM
	if (NakamaManager)
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

		JsonObject->SetStringField(TEXT("steamid"), NakamaManager->GetSteamID());
		JsonObject->SetStringField(TEXT("language"), TEXT("en"));
		JsonObject->SetStringField(TEXT("currency"), TEXT("USD"));
		JsonObject->SetNumberField(TEXT("bundleid"), BundleID);
		JsonObject->SetBoolField(TEXT("issandbox"), true);

		FString Payload;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

		if (NakamaClient)
			NakamaClient->RPC(UserSession, "steam_init_purchase", Payload, {}, {});
	}
# endif
}

void UNakamaShopManager::OnReceivedAllBundles(const FNakamaRPC& RPC)
{
	const FString JsonPayload = RPC.Payload;

	// Array to hold the bundles after conversion from JSON
	TArray<FBundleByID> BundleByIDArray;
	if (!FJsonObjectConverter::JsonArrayStringToUStruct<FBundleByID>(JsonPayload, &BundleByIDArray, 0, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse JSON."));
		return;
	}

	// Populate the map with data from the BundleByIDArray
	for (const FBundleByID& BundleByID : BundleByIDArray)
	{
		Bundles.Add(BundleByID.bundleid, BundleByID.bundle);
	}
}
