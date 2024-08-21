#pragma once
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "Characters/MCharacter.h"
#include "Controllers/MNPCController.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"
#include "StationaryActors/Outposts/MOutpostOccupiable.h"
#include "StationaryActors/Outposts/OutpostGenerators/MOutpostGenerator.h"
#include "BTTask_FindOccupiableElement.generated.h"

UCLASS()
class UBTTask_FindOccupiableElement : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindOccupiableElement()
	{
		NodeName = "Find Occupiable Element";
	}

	virtual FString GetStaticDescription() const override
	{
		return "Searches for not occupied AMOutpostOccupiable.\n"
		 " Uses a given Tag.\n"
		 " If found, sets ObjectToSet and LocationToSet";
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
						const auto& Elements = Outpost->GetElements();
						for (auto* Element : Elements)
						{
							if (auto* Occupiable = Cast<AMOutpostOccupiable>(Element))
							{
								if (Occupiable->AlreadyOccupying(MCharacter, TagToSearch) || !Occupiable->TryAddOccupant(MCharacter, TagToSearch))
								{
									NPCController->GetBlackboardComponent()->SetValueAsObject("Occupiable", Occupiable);
									NPCController->GetBlackboardComponent()->SetValueAsVector("OccupiableLocation", Occupiable->GetOccupyingLocation(MCharacter));

									return EBTNodeResult::Succeeded;
								}
							}
						}
					}
				}
			}
		}
		return EBTNodeResult::Failed;
	}

protected:
	UPROPERTY(EditAnywhere, Category=Blackboard)
	FName TagToSearch;
};

