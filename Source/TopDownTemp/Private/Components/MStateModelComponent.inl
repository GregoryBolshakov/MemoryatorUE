#pragma once

#include "MStateModelComponent.h"

inline void UMStateModelComponent::SetIsCommunicating(bool IN_IsCommunicating)
{
	if (IsCommunicating != IN_IsCommunicating)
	{
		IsCommunicating = IN_IsCommunicating;
		IsDirty = true;
	}
}

inline void UMStateModelComponent::SetIsDashing(bool IN_IsDashing)
{
	if (IsDashing != IN_IsDashing)
	{
		IsDashing = IN_IsDashing;
		IsDirty = true;
	}
}

inline void UMStateModelComponent::SetIsFighting(bool IN_IsFighting)
{
	if (IsFighting != IN_IsFighting)
	{
		IsFighting = IN_IsFighting;
		IsDirty = true;
	}
}

inline void UMStateModelComponent::SetIsMoving(bool IN_IsMoving)
{
	if (IsMoving != IN_IsMoving)
	{
		IsMoving = IN_IsMoving;
		IsDirty = true;
	}
}

inline void UMStateModelComponent::SetIsReversing(bool IN_IsReversing)
{
	if (IsReversing != IN_IsReversing)
	{
		IsReversing = IN_IsReversing;
		IsDirty = true;
	}
}

inline void UMStateModelComponent::SetIsSprinting(bool IN_IsSprinting)
{
	if (IsSprinting != IN_IsSprinting)
	{
		IsSprinting = IN_IsSprinting;
		IsDirty = true;
	}
}

inline void UMStateModelComponent::SetIsTakingDamage(bool IN_IsTakingDamage)
{
	if (IsTakingDamage != IN_IsTakingDamage)
	{
		IsTakingDamage = IN_IsTakingDamage;
		IsDirty = true;
	}
}

inline void UMStateModelComponent::SetIsTurningLeft(bool IN_IsTurningLeft)
{
	if (IsTurningLeft != IN_IsTurningLeft)
	{
		IsTurningLeft = IN_IsTurningLeft;
		IsDirty = true;
	}
}

inline void UMStateModelComponent::SetIsTurningRight(bool IN_IsTurningRight)
{
	if (IsTurningRight != IN_IsTurningRight)
	{
		IsTurningRight = IN_IsTurningRight;
		IsDirty = true;
	}
}
