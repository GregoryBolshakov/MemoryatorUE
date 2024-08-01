#include "MCharacterAnimInstance.h"

#include "Characters/MCharacter.h"
#include "Components/MStatsModelComponent.h"
#include "GameFramework/PawnMovementComponent.h"

void UMCharacterAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	if (const auto* MCharacter = Cast<AMCharacter>(TryGetPawnOwner()))
	{
		if (const auto* StatsComponent = MCharacter->GetStatsModelComponent())
		{
			// * 0.8 to account for errors due to inclined ground, difficult terrain, etc.
			SprintSpeedRequirement = StatsComponent->GetSprintSpeed() * 0.8;
		}
	}
}

void UMCharacterAnimInstance::PreUpdateAnimation(float DeltaSeconds)
{
	Super::PreUpdateAnimation(DeltaSeconds);

	if (const auto* MCharacter = Cast<AMCharacter>(TryGetPawnOwner()))
	{
		SpeedXY = MCharacter->GetVelocity().Size2D();

		// Calculate MovementGridLevel
		if (SpeedXY == 0)
			MovementGridLevel = 0;
		else
		{
			if (SpeedXY < SprintSpeedRequirement)
				MovementGridLevel = 1;
			else
				MovementGridLevel = 2;
		}
	}
}
