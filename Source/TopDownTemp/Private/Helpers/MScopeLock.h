#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/CriticalSection.h"
#include "Misc/SpinLock.h"

/**
 * Implements a scope lock using spin lock as a synchronization primitive.
 */
class FMScopeSpinLock
{
public:
	UE_NODISCARD_CTOR explicit FMScopeSpinLock(UE::FSpinLock* InSynchObject)
		: SynchObject(InSynchObject)
	{
		check(SynchObject);
		SynchObject->Lock();
	}

	~FMScopeSpinLock()
	{
		if (SynchObject)
		{
			SynchObject->Unlock();
		}
	}

	// Deleted copy constructor and assignment operator
	FMScopeSpinLock(const FMScopeSpinLock&) = delete;
	FMScopeSpinLock& operator=(const FMScopeSpinLock&) = delete;

private:
	// Holds the synchronization object to manage.
	UE::FSpinLock* SynchObject;
};

// TODO: Implement FMScopeSpinUnlock
