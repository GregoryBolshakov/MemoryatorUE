// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UObject/GCObject.h"
#include "UObject/ScriptInterface.h"

class NAKAMAMANAGER_API FNakamaManagerModule : public IModuleInterface, public FGCObject
{
public:
	virtual FString GetReferencerName() const override { return "NakamaManager"; }

	//~ IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override { return false; }
	virtual bool SupportsAutomaticShutdown() override { return false; }
	//~ End of IModuleInterface interface

	//~ FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	//~ End of FGCObject interface

	void Initialise(UObject* Outer, bool IN_bIsDedicatedServer);
	//bool IsInitialised() const;
	//bool IsAuthenticatedWithSteam() const;
	//bool UsingPreviewServer() const;

	class UNakamaManager* NakamaManager = nullptr;
};
