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

	if (const auto MobController = Cast<AMMobControllerBase>(NewResident->GetController()))
	{
		MobController->OnMovedIn(this);
	}
	//TODO: Handle player controller as well
	return true;
}

void AMOutpostHouse::MoveResidentOut(AMCharacter* OldResident)
{
	Residents.Remove(FName(OldResident->GetName()));
}
