#include "UserManagerClient.h"

#include "JsonObjectConverter.h"
#include "NakamaClient.h"
#include "NakamaManager.h"
#include "NakamaSession.h"
#include "Kismet/GameplayStatics.h"

void UUserManagerClient::Initialise(UNakamaManager* IN_NakamaManager)
{
	if (!IsValid(IN_NakamaManager))
	{
		check(false);
		return;
	}

	NakamaManager = IN_NakamaManager;
}

void UUserManagerClient::RequestIfHeroExists()
{
	if (NakamaManager && NakamaManager->NakamaClient)
	{
		NakamaManager->NakamaClient->RPC(NakamaManager->UserSession, "user_check_hero_exists", "", HeroExistsResponseDelegate, {});
	}
}

void UUserManagerClient::RequestCreateHero(const FString& Name, const FString& FullName = "", const FString& Email = "", bool TermsOfService = false, bool CompetitionRules = false, bool MarketingMaterial = false)
{
	if (!NakamaManager || !NakamaManager->NakamaClient)
		return;

	TSharedPtr<FJsonObject> FreshHeroRequestObject = MakeShareable(new FJsonObject());
	FreshHeroRequestObject->SetStringField(TEXT("name"), Name);
	//TODO: Add more data

	FString PayloadString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
	FJsonSerializer::Serialize(FreshHeroRequestObject.ToSharedRef(), Writer);

	// Call the create driver profile RPC
	NakamaManager->NakamaClient->RPC(NakamaManager->UserSession, "user_create_hero", PayloadString, CreateHeroResponseDelegate, {});

	// Update the existing profile
	NakamaManager->NakamaClient->UpdateAccount(NakamaManager->UserSession, Name, FullName, "", "EN", "", "", {}, {});
}

void UUserManagerClient::RequestHeroData()
{
	if (NakamaManager && NakamaManager->NakamaClient)
	{
		// Create the main JSON object and add the nested objects
		TSharedPtr<FJsonObject> ProfileObject = MakeShareable(new FJsonObject());
		ProfileObject->SetStringField(TEXT("email"), "");
		ProfileObject->SetStringField(TEXT("lastLogin"), FDateTime::UtcNow().ToIso8601());
		ProfileObject->SetBoolField(TEXT("requiresReview"), true);

		FString Payload;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
		FJsonSerializer::Serialize(ProfileObject.ToSharedRef(), Writer);

		FOnRPC HeroDataReceivedDelegate;
		HeroDataReceivedDelegate.AddDynamic(this, &UUserManagerClient::OnHeroDataReceived);
		NakamaManager->NakamaClient->RPC(NakamaManager->UserSession, "user_get_hero_data", Payload, HeroDataReceivedDelegate, {});
	}
}

void UUserManagerClient::SendHeroData()
{
	if (NakamaManager && NakamaManager->NakamaClient)
	{
		FString Payload;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
		if (!FJsonObjectConverter::UStructToJsonObjectString(FHero::StaticStruct(), &Hero, Payload, 0, 0))
		{
			check(false);
			return;
		}

		NakamaManager->NakamaClient->RPC(NakamaManager->UserSession, "user_set_hero_data", Payload, HeroDataSentDelegate, {});
	}
}

void UUserManagerClient::OnHeroDataReceived(const FNakamaRPC& RPC)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(RPC.Payload);

	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		// Parsing main Hero data
		Hero.Name = JsonObject->GetStringField("name");
		Hero.Experience = JsonObject->GetIntegerField("experience");
		Hero.Level = JsonObject->GetIntegerField("level");

		HeroDataUpdatedDelegate.Broadcast();
	}
	else
	{
		check(false);
	}
}
