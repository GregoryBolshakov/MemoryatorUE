#pragma once

#include "MStatsModelComponent.h"

inline void UMStatsModelComponent::SetCanRetreat(bool IN_CanRetreat)
{
	if (bCanRetreat != IN_CanRetreat)
	{
		bCanRetreat = IN_CanRetreat;
		IsDirty = true;
	}
}

inline void UMStatsModelComponent::SetForgetEnemyRange(float IN_ForgetEnemyRange)
{
	if (ForgetEnemyRange != IN_ForgetEnemyRange)
	{
		ForgetEnemyRange = IN_ForgetEnemyRange;
		IsDirty = true;
	}
}

inline void UMStatsModelComponent::SetHealth(float IN_Health)
{
	if (Health.Value != IN_Health)
	{
		Health.Value = IN_Health;
		IsDirty = true;
	}
}

inline void UMStatsModelComponent::SetMeleeSpread(float IN_MeleeSpread)
{
	if (MeleeSpread != IN_MeleeSpread)
	{
		MeleeSpread = IN_MeleeSpread;
		IsDirty = true;
	}
}

inline void UMStatsModelComponent::SetRetreatRange(float IN_RetreatRange)
{
	if (RetreatRange != IN_RetreatRange)
	{
		RetreatRange = IN_RetreatRange;
		IsDirty = true;
	}
}

inline void UMStatsModelComponent::SetSightRange(float IN_SightRange)
{
	if (SightRange != IN_SightRange)
	{
		SightRange = IN_SightRange;
		IsDirty = true;
	}
}

inline void UMStatsModelComponent::SetSprintSpeed(float IN_SprintSpeed)
{
	if (SprintSpeed != IN_SprintSpeed)
	{
		SprintSpeed = IN_SprintSpeed;
		IsDirty = true;
	}
}

inline void UMStatsModelComponent::SetStrength(float IN_Strength)
{
	if (Strength != IN_Strength)
	{
		Strength = IN_Strength;
		IsDirty = true;
	}
}

inline void UMStatsModelComponent::SetTimeBeforeSprint(float IN_TimeBeforeSprint)
{
	if (TimeBeforeSprint != IN_TimeBeforeSprint)
	{
		TimeBeforeSprint = IN_TimeBeforeSprint;
		IsDirty = true;
	}
}

inline void UMStatsModelComponent::SetWalkSpeed(float IN_WalkSpeed)
{
	if (WalkSpeed != IN_WalkSpeed)
	{
		WalkSpeed = IN_WalkSpeed;
		IsDirty = true;
	}
}
