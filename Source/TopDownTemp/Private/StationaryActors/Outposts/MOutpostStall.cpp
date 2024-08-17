#include "MOutpostStall.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/MCharacter.h"
#include "Controllers/MMobControllerBase.h"
#include "Controllers/MNPCController.h"

void AMOutpostStall::ResetTrader()
{
	if (auto* MNPCController = Cast<AMNPCController>(Trader->GetController()))
	{
		MNPCController->OnRetreatDelegate.RemoveAll(this);
		MNPCController->GetBlackboardComponent()->SetValueAsObject("Stall", nullptr);
		MNPCController->GetBlackboardComponent()->ClearValue("StallLocation");
	}
	Trader = nullptr;
}

FVector AMOutpostStall::GetEntryPoint() const
{
	TArray<USceneComponent*> ChildComponents;
	GetComponents(ChildComponents);
	for (const auto* ChildComponent : ChildComponents)
	{
		if (ChildComponent->GetFName() == FName("EntryPoint"))
		{
			return ChildComponent->GetComponentLocation(); // TODO: Gather an array of entry points instead
		}
	}
	check(false);
	return FVector::Zero();
}
