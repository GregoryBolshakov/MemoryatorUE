#pragma once
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AlwaysSuccess.generated.h"

UCLASS()
class UBTTask_AlwaysSuccess : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AlwaysSuccess()
	{
		NodeName = "Always Success";
	}

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override
	{
		return EBTNodeResult::Succeeded;
	}
};

