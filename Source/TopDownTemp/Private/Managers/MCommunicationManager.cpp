#include "MCommunicationManager.h"

#include "Characters/MCharacter.h"
#include "GenericTeamAgentInterface.h"

namespace
{
	ETeamAttitude::Type CustomTeamAttitudeSolver(FGenericTeamId A, FGenericTeamId B)
	{
		// Normalize the order of A and B to ensure consistency
		if (A.GetId() > B.GetId())
		{
			Swap(A, B);
		}

		if (A == GetTeamIdByEnum(EMTeamID::Nightmares) && B == GetTeamIdByEnum(EMTeamID::Witches))
		{
			return ETeamAttitude::Neutral;
		}

		// TODO: Put other cross-faction attitudes here

		return A != B ? ETeamAttitude::Hostile : ETeamAttitude::Friendly;
	}
}

void AMCommunicationManager::BeginPlay()
{
	Super::BeginPlay();

	FGenericTeamId::SetAttitudeSolver(CustomTeamAttitudeSolver);
}
