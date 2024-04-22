#pragma once

#include "CoreMinimal.h"
#include "MUid.generated.h"

/** Saved actors might depend on other saved actors when loading. They use this ID.\n
*   Provides the overall uniqueness of actors in the world
*   Grants total uniqueness across different game launches */
USTRUCT()
struct FMUid
{
	GENERATED_BODY()

	/** When the game is saved, it makes sense to write only GridOfActors's objects to memory.\n
	 * It means not all the objects from USaveGameWorld::SavedGrid get updated in the save.\n
	 * Unique names are created by a simple int32 counter (NumberUniqueIndex resets every time game launches),
	 * and objects in the save can be selectively deleted.
	 * Therefore intersection of unique identifiers is possible.\n\n
	 * This ID makes all objects unique across different game sessions. */
	UPROPERTY()
	int32 LaunchId = MIN_int32;

	/** Unique object identifier within this game session */
	UPROPERTY()
	int32 ObjectId = MIN_int32;

	friend bool operator==(const FMUid& lhs, const FMUid& rhs)
	{
		return lhs.LaunchId == rhs.LaunchId && lhs.ObjectId == rhs.ObjectId;
	}

	friend uint32 GetTypeHash(const FMUid& uid)
	{
		return HashCombine(::GetTypeHash(uid.LaunchId), ::GetTypeHash(uid.ObjectId));
	}
};

inline bool IsUidValid(const FMUid& Uid) { return Uid.ObjectId > MIN_int32; }