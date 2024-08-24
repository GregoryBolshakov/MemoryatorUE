#include "MCharacterMovementComponent.h"

#include "MStatsModelComponent.h"
#include "Characters/MCharacter.h"

UMCharacterMovementComponent::UMCharacterMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Hide the MaxStepHeight property from the editor
	if (FProperty* Prop = FindFProperty<FProperty>(UCharacterMovementComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UCharacterMovementComponent, MaxStepHeight)))
	{
		Prop->SetPropertyFlags(CPF_DisableEditOnInstance);
	}
}

void UMCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (const auto* MCharacter = Cast<AMCharacter>(GetOwner()))
	{
		if (const auto* StatsModel = MCharacter->GetStatsModelComponent())
		{
			if (MaxWalkSpeed >= StatsModel->GetSprintSpeed())
			{
				MaxStepHeight = MaxStepHeightSprint;
				return;
			}
		}
	}
	MaxStepHeight = MaxStepHeightSlowMovement;
}
