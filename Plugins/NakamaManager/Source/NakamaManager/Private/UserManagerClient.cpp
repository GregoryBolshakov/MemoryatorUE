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

	// Create JSON objects for the nested structs
	TSharedPtr<FJsonObject> VerifiedObject = MakeShareable(new FJsonObject());
	VerifiedObject->SetBoolField(TEXT("email"), !Email.IsEmpty());
	VerifiedObject->SetBoolField(TEXT("account"), !FullName.IsEmpty());
	VerifiedObject->SetBoolField(TEXT("msg"), !Name.IsEmpty());

	TSharedPtr<FJsonObject> OptinsObject = MakeShareable(new FJsonObject());
	OptinsObject->SetBoolField(TEXT("competitionRules"), CompetitionRules);
	OptinsObject->SetBoolField(TEXT("emailMarketing"), MarketingMaterial);
	OptinsObject->SetBoolField(TEXT("termsOfService"), TermsOfService);

	// Create the main JSON object and add the nested objects
	TSharedPtr<FJsonObject> ProfileObject = MakeShareable(new FJsonObject());
	ProfileObject->SetStringField(TEXT("email"), Email);
	ProfileObject->SetObjectField(TEXT("verified"), VerifiedObject);
	ProfileObject->SetObjectField(TEXT("optins"), OptinsObject);

	FString PayloadString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
	FJsonSerializer::Serialize(ProfileObject.ToSharedRef(), Writer);

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

		FString PayloadString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
		FJsonSerializer::Serialize(ProfileObject.ToSharedRef(), Writer);
		
		FOnRPC HeroDataReceivedDelegate;
		HeroDataReceivedDelegate.AddDynamic(this, &UUserManagerClient::OnHeroDataReceived);
		NakamaManager->NakamaClient->RPC(NakamaManager->UserSession, "user_get_hero_data", PayloadString, HeroDataReceivedDelegate, {});
	}
}

void UUserManagerClient::OnHeroDataReceived(const FNakamaRPC& RPC)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(RPC.Payload);

	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		// Parsing main Hero data
		Hero.Id = JsonObject->GetStringField("id");
		Hero.Name = JsonObject->GetStringField("name");
		Hero.SteamId = JsonObject->GetStringField("steamId");
		Hero.Experience = JsonObject->GetIntegerField("experience");
		Hero.Level = JsonObject->GetIntegerField("level");

		// Parsing DateTimes
		FDateTime::ParseIso8601(*JsonObject->GetStringField("lastNameChange"), Hero.LastNameChange);

		// Parsing nested Profile object
		TSharedPtr<FJsonObject> ProfileObject = JsonObject->GetObjectField("profile");
		if (ProfileObject.IsValid())
		{
			Hero.Profile.Email = ProfileObject->GetStringField("email");

			FDateTime::ParseIso8601(*ProfileObject->GetStringField("lastLogin"), Hero.Profile.LastLogin);
            
			Hero.Profile.RequiresReview = ProfileObject->GetBoolField("requiresReview");
		}

		HeroDataUpdatedDelegate.Broadcast();
	}
	else
	{
		check(false);
	}
}

