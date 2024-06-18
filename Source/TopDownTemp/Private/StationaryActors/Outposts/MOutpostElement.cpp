#include "MOutpostElement.h"

#include "Framework/MGameMode.h"
#include "Managers/SaveManager/MWorldSaveTypes.h"
#include "StationaryActors/Outposts/OutpostGenerators/MOutpostGenerator.h"

FMActorSaveData AMOutpostElement::GetSaveData() const
{
	auto MActorSD = Super::GetSaveData();
	// Save the OwnerOutpost Uid. Normally should always have it
	if (OwnerOutpost)
	{
		if (const auto* OutpostMetadata = AMGameMode::GetMetadataManager(this)->Find(FName(OwnerOutpost->GetName())))
		{
			MActorSD.DependenciesUid.Add("Outpost", OutpostMetadata->Uid);
		}
	}

	return MActorSD;
}

void AMOutpostElement::BeginLoadFromSD(const FMActorSaveData& MActorSD)
{
	Super::BeginLoadFromSD(MActorSD);
	auto* SaveManager = AMGameMode::GetSaveManager(this);
	// Load the OwnerOutpost from save. Normally should always have it
	if (const auto* pOutpostUid = MActorSD.DependenciesUid.Find("Outpost"); pOutpostUid && IsUidValid(*pOutpostUid))
	{
		auto* Outpost = Cast<AMOutpostGenerator>(SaveManager->LoadMActorAndClearSD(*pOutpostUid));
		OwnerOutpost = Outpost;
		check(OwnerOutpost);
	}
}
