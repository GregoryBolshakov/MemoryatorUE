#include "MGameInstance.h"
#include "NakamaManagerModule.h"

UMGameInstance::UMGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ItemsDataAsset(nullptr)
{
	const bool bIsCDO = (0 != (GetFlags() & EObjectFlags::RF_ClassDefaultObject));
	if (!bIsCDO)
	{
		NakamaManagerModule = static_cast<FNakamaManagerModule*>(FModuleManager::Get().GetModule(TEXT("NakamaManager")));
		ensure(NakamaManagerModule);
		NakamaManagerModule->Initialise(this, IsDedicatedServerInstance());
	}
}