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
	// 4 Dash
	Dash				UMETA(DisplayName = "Dash"),
	// 5 Jump
	Jump				UMETA(DisplayName = "Jump"),
	// 6 PrimaryAttack
	PrimaryAttack		UMETA(DisplayName = "Primary Attack"),
	// 7 SecondaryAttack
	SecondaryAttack		UMETA(DisplayName = "Secondary Attack"),
	// 8 Alternate Attack
	AlternateAttack		UMETA(DisplayName = "Alternate Attack"),
	// 9 Reload
	Reload				UMETA(DisplayName = "Reload"),
	// 10 NextWeapon
	NextWeapon			UMETA(DisplayName = "Next Weapon"), 
	// 11 PrevWeapon
	PrevWeapon			UMETA(DisplayName = "Previous Weapon"),
	// 12 Interact
	Interact			UMETA(DisplayName = "Interact")
};
