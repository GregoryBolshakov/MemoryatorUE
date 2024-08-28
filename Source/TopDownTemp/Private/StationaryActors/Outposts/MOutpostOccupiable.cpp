#include "MOutpostOccupiable.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/MCharacter.h"
#include "Controllers/MNPCController.h"

AMCharacter* AMOutpostOccupiable::GetAnyOccupant() const
{
	for (const auto* OccupiablePoint : OccupiablePoints)
	{
		if (IsValid(OccupiablePoint->Occupant))
		{
			return OccupiablePoint->Occupant;
		}
	}
	return nullptr;
}

bool AMOutpostOccupiable::TryAddOccupant(AMCharacter* NewOccupant, FName Tag)
{
	for (auto* OccupiablePoint : OccupiablePoints)
	{
		if (!OccupiablePoint->Occupant && OccupiablePoint->ComponentHasTag(Tag))
		{
			OccupiablePoint->Occupant = NewOccupant;
			return true;
		}
	}
	return false;
}

bool AMOutpostOccupiable::AlreadyOccupying(const AMCharacter* Occupant, FName Tag)
{
	for (auto* OccupiablePoint : OccupiablePoints)
	{
		if (OccupiablePoint->Occupant == Occupant && OccupiablePoint->ComponentHasTag(Tag))
		{
			return true;
		}
	}
	return false;
}

FVector AMOutpostOccupiable::GetOccupyingLocation(const AMCharacter* Occupant)
{
	for (auto* OccupiablePoint : OccupiablePoints)
	{
		if (OccupiablePoint->Occupant == Occupant)
		{
			return OccupiablePoint->GetComponentLocation();
		}
	}
	check(false);
	return {};
}

// So far, there's no need to manually remove occupants. Occupants tend to leave their points themselves
/*void AMOutpostOccupiable::RemoveOccupant(AMCharacter* Occupant)
{
	for (auto* OccupiablePoint : OccupiablePoints)
	{
		if (OccupiablePoint->Occupant == Occupant)
		{
			call controller's StopOccupying()
		}
	}
}*/

void AMOutpostOccupiable::OnOccupantRemoved(AMCharacter* Occupant)
{
	for (auto* OccupiablePoint : OccupiablePoints)
	{
		if (OccupiablePoint->Occupant == Occupant)
		{
			Occupant = nullptr;
		}
	}
}

void AMOutpostOccupiable::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	OccupiablePoints.Empty();
	for (auto* OccupiablePoint : K2_GetComponentsByClass(UMOccupiablePointComponent::StaticClass()))
	{
		OccupiablePoints.Add(Cast<UMOccupiablePointComponent>(OccupiablePoint));
	}
}
