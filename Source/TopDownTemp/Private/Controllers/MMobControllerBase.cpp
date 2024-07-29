#include "MMobControllerBase.h"
#include "Characters/MCharacter.h"
#include "Framework/MGameMode.h"
#include "Managers/MCommunicationManager.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

AMMobControllerBase::AMMobControllerBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
	//PrimaryActorTick.TickInterval = 0.5f; // TODO: Lower tick rate basing on distance/whatnot
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

	if (CurrentBehavior == EMobBehaviors::Idle)
	{
		// Communication check
		if (IsPlayerSpeakingToMe() && GetAttitudeToPlayer() != ETeamAttitude::Hostile)
		{
			SetIdleBehavior(pWorld, MyCharacter); // To reset all possible idle timers
			return; // If the mob is having a conversation with player, do nothing and keep standing
		}
		// Wait some time, then find a random point within the village to go
		DoIdleBehavior(*pWorld, *MyCharacter);
	}
	if (CurrentBehavior == EMobBehaviors::Walk)
	{
		// Communication check
		if (IsPlayerSpeakingToMe() && GetAttitudeToPlayer() != ETeamAttitude::Hostile)
		{
			SetIdleBehavior(pWorld, MyCharacter); // To reset all possible idle timers
			return; // If the mob is having a conversation with player, do nothing and keep standing
		}
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
	if (CurrentBehavior == EMobBehaviors::Fight)
	{
		// Attack the victim
		DoFightBehavior(*pWorld, *MyCharacter);
	}
	if (CurrentBehavior == EMobBehaviors::Chase)
	{
		// Chase the victim
		DoChaseBehavior(*pWorld, *MyCharacter);
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

bool AMMobControllerBase::IsPlayerSpeakingToMe()
{
	const auto MyCharacter = Cast<AMCharacter>(GetPawn());
	if (!MyCharacter) { check(false); return false; }

	const auto pWorld = GetWorld();
	if (!pWorld) { check(false); return false; }

	if (const auto CommunicationManager = AMGameMode::GetCommunicationManager(this))
	{
		if (const auto InterlocutorCharacter = CommunicationManager->GetInterlocutorCharacter(); InterlocutorCharacter && InterlocutorCharacter == MyCharacter)
		{
			return true;
		}
	}

	return false;
}

ETeamAttitude::Type AMMobControllerBase::GetAttitudeToPlayer() const
{
	const auto pWorld = GetWorld();
	const auto MyMCharacter = Cast<AMCharacter>(GetPawn());
	if (!pWorld || !MyMCharacter) { check(false); return ETeamAttitude::Type::Neutral; }

	if (const auto PlayerCharacter = UGameplayStatics::GetPlayerCharacter(pWorld, 0))
	{
		return FGenericTeamId::GetAttitude(GetPawn(), PlayerCharacter);
	}
	check(false);
	return ETeamAttitude::Type::Neutral;
}
