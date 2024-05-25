#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTopDownTemp, Log, All);

/** The ASC allows to directly bind input actions to it and assign those inputs to GameplayAbilities when you grant them.
 * Input actions assigned to GameplayAbilities automatically activate those GameplayAbilities when pressed
 * if the GameplayTag requirements are met.
 * Assigned input actions are required to use the built-in AbilityTasks that respond to input.*/
UENUM(BlueprintType)
enum class EMAbilityInputID : uint8
{
	// 0 None
	None				UMETA(DisplayName = "None"),
	// 1 Confirm
	Confirm				UMETA(DisplayName = "Confirm"),
	// 2 Cancel
	Cancel				UMETA(DisplayName = "Cancel"),
	// 3 Sprint
	Sprint				UMETA(DisplayName = "Sprint"),
	// 4 Jump
	Dash				UMETA(DisplayName = "Dash"),
	// 5 PrimaryFire
	PrimaryFire			UMETA(DisplayName = "Primary Fire"),
	// 6 SecondaryFire
	SecondaryFire		UMETA(DisplayName = "Secondary Fire"),
	// 7 Alternate Fire
	AlternateFire		UMETA(DisplayName = "Alternate Fire"),
	// 8 Reload
	Reload				UMETA(DisplayName = "Reload"),
	// 9 NextWeapon
	NextWeapon			UMETA(DisplayName = "Next Weapon"), 
	// 10 PrevWeapon
	PrevWeapon			UMETA(DisplayName = "Previous Weapon"),
	// 11 Interact
	Interact			UMETA(DisplayName = "Interact")
};
