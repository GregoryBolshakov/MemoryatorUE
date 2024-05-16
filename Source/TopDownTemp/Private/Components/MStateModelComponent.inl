#pragma once

#include "MStateModelComponent.h"

inline void UMStateModelComponent::SetIsDashing(bool IN_IsDashing)
{
	if (IsDashing != IN_IsDashing)
	{
		IsDashing = IN_IsDashing;
		MarkDirty();
	}
}

inline void UMStateModelComponent::SetIsFighting(bool IN_IsFighting)
{
	if (IsFighting != IN_IsFighting)
	{
		IsFighting = IN_IsFighting;
		MarkDirty();
	}
}

inline void UMStateModelComponent::SetIsMoving(bool IN_IsMoving)
{
	if (IsMoving != IN_IsMoving)
	{
		IsMoving = IN_IsMoving;
		MarkDirty();
	}
}

inline void UMStateModelComponent::SetIsSprinting(bool IN_IsSprinting)
{
	if (IN_IsSprinting)
	{
		IsSprinting = IN_IsSprinting;
		MarkDirty();
	}
}

inline void UMStateModelComponent::SetIsTakingDamage(bool IN_IsTakingDamage)
{
	if (IsTakingDamage != IN_IsTakingDamage)
	{
		IsTakingDamage = IN_IsTakingDamage;
		MarkDirty();
	}
}
