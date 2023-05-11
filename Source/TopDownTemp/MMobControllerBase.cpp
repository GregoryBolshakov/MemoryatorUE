#include "MMobControllerBase.h"
#include "MCharacter.h"
#include "MCommunicationManager.h"
#include "MWorldGenerator.h"
#include "MWorldManager.h"
#include "Kismet/GameplayStatics.h"

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

	if (CurrentBehavior == EMobBehaviors::Idle)
	{
		// Communication check
		if (IsPlayerSpeakingToMe() && GetRelationshipWithPlayer() != ERelationType::Enemy)
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
		if (IsPlayerSpeakingToMe() && GetRelationshipWithPlayer() != ERelationType::Enemy)
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

	if (const auto WorldManager = pWorld->GetSubsystem<UMWorldManager>())
	{
		if (const auto WorldGenerator = WorldManager->GetWorldGenerator())
		{
			if (const auto CommunicationManager = WorldGenerator->GetCommunicationManager())
			{
				if (const auto InterlocutorCharacter = CommunicationManager->GetInterlocutorCharacter(); InterlocutorCharacter && InterlocutorCharacter == MyCharacter)
				{
					return true;
				}
			}
		}
	}

	return false;
}

ERelationType AMMobControllerBase::GetRelationshipWithPlayer()
{
	const auto pWorld = GetWorld();
	if (!pWorld) { check(false); return ERelationType::Neutral; }

	if (const auto PlayerCharacter = UGameplayStatics::GetPlayerCharacter(pWorld, 0))
	{
		if (const auto RelationshipWithPlayer = RelationshipMap.Find(PlayerCharacter->GetClass()))
		{
			return *RelationshipWithPlayer;
		}
	}
	check(false);
	return ERelationType::Neutral;
}
