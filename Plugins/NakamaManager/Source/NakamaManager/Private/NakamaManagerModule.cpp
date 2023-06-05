// Copyright Epic Games, Inc. All Rights Reserved.

#include "NakamaManagerModule.h"

#include "NakamaManager.h"

#define LOCTEXT_NAMESPACE "FNakamaManagerModule"

void FNakamaManagerModule::StartupModule() {}

void FNakamaManagerModule::ShutdownModule() {}

void FNakamaManagerModule::AddReferencedObjects(FReferenceCollector& Collector) { Collector.AddReferencedObject(NakamaManager); }

void FNakamaManagerModule::Initialise(UObject* Outer, bool IN_bIsDedicatedServer)
{
	// todo
	if (!IsValid(NakamaManager))
	{
		NakamaManager = NewObject<UNakamaManager>(Outer, "NakamaManager");
		NakamaManager->Initialise(IN_bIsDedicatedServer);
	}
}

//bool FNakamaManagerModule::IsInitialised() const { return NakamaManager ? NakamaManager->IsInitialised() : false; }

//bool FNakamaManagerModule::IsAuthenticatedWithSteam() const { return NakamaManager ? NakamaManager->IsAuthenticatedWithSteam() : false; }

//bool FNakamaManagerModule::UsingPreviewServer() const { return NakamaManager ? NakamaManager->UsingPreviewServer() : false; }

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNakamaManagerModule, NakamaManager)
