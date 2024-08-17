#pragma once
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "Characters/MCharacter.h"
#include "Controllers/MNPCController.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"
#include "StationaryActors/Outposts/MOutpostStall.h"
#include "StationaryActors/Outposts/OutpostGenerators/MOutpostGenerator.h"
#include "BTService_FindStallLocation.generated.h"

UCLASS()
class UBTTask_FindStallLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindStallLocation()
	{
		NodeName = "Find Stall Location";
	}

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override
	{
		if (auto* NPCController = Cast<AMNPCController>(OwnerComp.GetOwner()))
		{
			if (auto* MCharacter = Cast<AMCharacter>(NPCController->GetPawn()))
			{
				if (const auto* House = MCharacter->GetHouse())
				{
					if (const auto* Outpost = House->GetOwnerOutpost())
					{
						const auto& Stalls = Outpost->GetStalls();
						for (const auto& [Name, Stall] : Stalls)
						{
							if (!Stall->GetTrader())
							{
								Stall->SetTrader(MCharacter);
								NPCController->OnRetreatDelegate.AddDynamic(Stall, &AMOutpostStall::ResetTrader);

								NPCController->GetBlackboardComponent()->SetValueAsObject("Stall", Stall);
								NPCController->GetBlackboardComponent()->SetValueAsVector("StallLocation", Stall->GetEntryPoint());

								return EBTNodeResult::Succeeded;
							}
						}
					}
				}
			}
		}
		return EBTNodeResult::Failed;
	}
};

