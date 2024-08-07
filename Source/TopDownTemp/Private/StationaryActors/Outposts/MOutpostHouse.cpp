#include "MOutpostHouse.h"
#include "Characters/MCharacter.h"
#include "Controllers/MMobControllerBase.h"

bool AMOutpostHouse::MoveResidentIn(AMCharacter* NewResident)
{
	// Remove invalid residents
	TArray<FName> KeysToRemove;
	for (const auto& [Name, Resident] : Residents)
	{
		if (!IsValid(Resident))
		{
			KeysToRemove.Add(Name);
		}
	}
	for (const auto Name : KeysToRemove)
	{
		Residents.Remove(Name);
	}

	// Capacity check
	if (Residents.Num() >= Capacity)
	{
		return false;
	}

	Residents.Add({FName(NewResident->GetName()), NewResident});

	NewResident->OnMovedIn(this);

	//TODO: Handle player controller as well
	return true;
}

void AMOutpostHouse::MoveResidentOut(AMCharacter* OldResident)
{
	OldResident->OnMovedOut();
	Residents.Remove(FName(OldResident->GetName()));
}

FVector AMOutpostHouse::GetEntryPoint() const
{
	TArray<USceneComponent*> ChildComponents;
	GetComponents(ChildComponents);
	for (const auto* ChildComponent : ChildComponents)
	{
		if (ChildComponent->GetFName() == FName("EntryPoint"))
		{
			return ChildComponent->GetComponentLocation(); // TODO: Gather an array of entry points instead
		}
	}
	check(false);
	return FVector::Zero();
}
