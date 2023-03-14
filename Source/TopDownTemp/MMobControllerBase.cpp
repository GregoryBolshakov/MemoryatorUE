#include "MMobControllerBase.h"
#include "MCharacter.h"

AMMobControllerBase::AMMobControllerBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
}

void AMMobControllerBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const auto pWorld = GetWorld();
	if (!pWorld)
	{
		check(false);
		return;
	}

	const auto MyCharacter = Cast<AMCharacter>(GetPawn());
	if (!MyCharacter)
	{
		check(false);
		return;
	}

	PreTick(DeltaSeconds, *pWorld, *MyCharacter);

	// Reset ForcedGazeVector, it might be set in one of behavior functions 
	MyCharacter->SetForcedGazeVector(FVector::ZeroVector);

	if (CurrentBehavior == EMobBehaviors::Idle)
	{
		// Wait some time, then find a random point within the village to go
		DoIdleBehavior(*pWorld, *MyCharacter);
	}
	if (CurrentBehavior == EMobBehaviors::Walk)
	{
		// Go to the destination point
		DoWalkBehavior(*pWorld, *MyCharacter);
	}
	if (CurrentBehavior == EMobBehaviors::Retreat)
	{
		// Run away from the victim until move away by a set range
		DoRetreatBehavior(*pWorld, *MyCharacter);
	}
	if (CurrentBehavior == EMobBehaviors::Hide)
	{
		// Run away from the victim until move away by a set range
		DoHideBehavior(*pWorld, *MyCharacter);
	}
}

void AMMobControllerBase::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	AAIController::OnMoveCompleted(RequestID, Result);

	const auto pWold = GetWorld();
	if (!pWold)
	{
		check(false);
		return;
	}

	const auto MyCharacter = Cast<AMCharacter>(GetPawn());
	if (!MyCharacter)
	{
		check(false);
		return;
	}

	if (!Result.IsSuccess() || !OnMoveCompletedDelegate.IsBound())
	{
		//SetIdleBehavior(*pWold, *MyCharacter);
		return;
	}

	OnMoveCompletedDelegate.Execute();
}
