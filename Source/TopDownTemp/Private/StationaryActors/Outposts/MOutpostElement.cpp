#include "MOutpostElement.h"

#include "Components/BoxComponent.h"
#include "Framework/MGameMode.h"
#include "Helpers/M2DRepresentationBlueprintLibrary.h"
#include "Managers/SaveManager/MWorldSaveTypes.h"
#include "StationaryActors/Outposts/OutpostGenerators/MOutpostGenerator.h"

AMOutpostElement::AMOutpostElement(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ScopeForShifting = CreateDefaultSubobject<UBoxComponent>("ScopeForShifting");
	// A handy extent to start working with. Adjust in blueprint if needed
	ScopeForShifting->SetBoxExtent({200.f, 200.f, 200.f});
	ScopeForShifting->SetRelativeLocation({0.f, 0.f, 150.f});
	ScopeForShifting->SetCollisionProfileName(FName("BuildingGeneratorBounds"));
	ScopeForShifting->SetupAttachment(RootComponent);
}

void AMOutpostElement::RotateContentToPoint(const FVector& Point) const
{
	TArray<USceneComponent*> AffectedChildren;
	ScopeForShifting->GetChildrenComponents(false, AffectedChildren);
	for (auto* AffectedChild : AffectedChildren)
	{
		const auto Location = AffectedChild->GetComponentLocation();
		AffectedChild->SetRelativeRotation(UM2DRepresentationBlueprintLibrary::GetRotationTowardPoint(Location, Point));
	}
}

void AMOutpostElement::RotateContentRandomly() const
{
	TArray<USceneComponent*> AffectedChildren;
	ScopeForShifting->GetChildrenComponents(false, AffectedChildren);
	for (auto* AffectedChild : AffectedChildren)
	{
		const auto RandomRotation = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f);
		AffectedChild->SetRelativeRotation(RandomRotation);
	}
}

void AMOutpostElement::RotateAndMoveContentRandomly() const
{
	TArray<USceneComponent*> AffectedChildren;
	ScopeForShifting->GetChildrenComponents(false, AffectedChildren);
	for (auto* AffectedChild : AffectedChildren)
	{
		const auto RandomRotation = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f);
		AffectedChild->SetRelativeRotation(RandomRotation);

		const auto ComponentBounds = AffectedChild->Bounds;
		FBoxSphereBounds RandomShiftingBounds;
		GetActorBounds(true, RandomShiftingBounds.Origin, RandomShiftingBounds.BoxExtent, true);

		const auto ComponentLowerBound = ComponentBounds.Origin - ComponentBounds.BoxExtent;
		const auto ComponentUpperBound = ComponentBounds.Origin + ComponentBounds.BoxExtent;

		const auto RandomShiftingLowerBound = RandomShiftingBounds.Origin - RandomShiftingBounds.BoxExtent;
		const auto RandomOffsetUpperBound = RandomShiftingBounds.Origin + RandomShiftingBounds.BoxExtent;

		const FVector RandomOffset = FVector(
			FMath::RandRange(RandomShiftingLowerBound.X - ComponentLowerBound.X, RandomOffsetUpperBound.X - ComponentUpperBound.X),
			FMath::RandRange(RandomShiftingLowerBound.Y - ComponentLowerBound.Y, RandomOffsetUpperBound.Y - ComponentUpperBound.Y),
			0.f);
		AffectedChild->SetRelativeLocation(AffectedChild->GetRelativeLocation() + RandomOffset);
	}
}

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
