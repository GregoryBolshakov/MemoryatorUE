#include "ShopManagerClient.h"

#include "JsonObjectConverter.h"
#include "NakamaClient.h"
#include "NakamaManager.h"
#include "NakamaSession.h"
#include "Kismet/GameplayStatics.h"

void UShopManagerClient::Initialise(UNakamaClient* IN_NakamaClient, UNakamaSession* IN_UserSession, UNakamaManager* IN_NakamaManager)
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
	OnReceivedAllBundlesSuccess.AddDynamic(this, &UShopManagerClient::OnReceivedAllBundles);
	NakamaClient->RPC(UserSession, "get_all_bundles", "", OnReceivedAllBundlesSuccess, {});
}

void UShopManagerClient::BuyBundle(const uint32 BundleID)
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

void UShopManagerClient::OnReceivedAllBundles(const FNakamaRPC& RPC)
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

# ifdef USING_STEAM
void UShopManagerClient::OnMicroTxnAuthorizationResponse(MicroTxnAuthorizationResponse_t* pParam)
{
	if (pParam->m_bAuthorized)
	{
		if (NakamaManager)
		{
			TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

			JsonObject->SetStringField(TEXT("orderid"), FString::FromInt(pParam->m_ulOrderID));
			JsonObject->SetNumberField(TEXT("appid"), pParam->m_unAppID);

			FString Payload;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
			FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

			FOnRPC OnTxnFinalizedDelegate;
			OnTxnFinalizedDelegate.AddDynamic(this, &UShopManagerClient::OnTxnFinalized);
			if (NakamaClient)
				NakamaClient->RPC(UserSession, "steam_finalize_purchase", Payload, OnTxnFinalizedDelegate, {});
		}
	}
	else
	{
		//TODO: Handle this nicely
	}
}
#endif

void UShopManagerClient::OnTxnFinalized(const FNakamaRPC& RPC)
{
	const FString JsonPayload = RPC.Payload;

	// Array to hold the bundles after conversion from JSON
	FBundle Bundle;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct<FBundle>(JsonPayload, &Bundle, 0, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse JSON."));
		return;
	}

	OnBundleTxnFinalizedDelegate.ExecuteIfBound(Bundle);
}
