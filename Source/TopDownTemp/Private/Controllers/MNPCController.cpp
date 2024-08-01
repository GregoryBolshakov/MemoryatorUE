#include "MNPCController.h"
#include "AI/MTeamIDs.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/MCharacter.h"
#include "Components/MCommunicationComponent.h"
#include "Components/MIsActiveCheckerComponent.h"
#include "Components/MStateModelComponent.h"

#include "Perception/AIPerceptionComponent.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"
#include "StationaryActors/Outposts/OutpostGenerators/MOutpostGenerator.h"

AMNPCController::AMNPCController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// All NPCs ticks reduced to half a second by default
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
	//PrimaryActorTick.TickInterval = 0.5f; // TODO: Lower tick rate basing on distance/whatnot
}

FGenericTeamId AMNPCController::GetGenericTeamId() const
{
	if (const auto* MCharacter = Cast<AMCharacter>(GetPawn()))
	{
		return MCharacter->GetTeamID();
	}
	return GetTeamIdByEnum(EMTeamID::None);
}

ETeamAttitude::Type AMNPCController::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (const auto* MyMCharacter = Cast<AMCharacter>(GetPawn()))
	{
		// First check custom attitudes
		if (const auto* Attitude = MyMCharacter->GetCustomAttitudes().Find(Other.GetClass()))
		{
			return *Attitude;
		}
		// If not present, follow the general team-based rules set by AMCommunicationManager::CustomTeamAttitudeSolver
		return Super::GetTeamAttitudeTowards(Other);
	}
	return ETeamAttitude::Type::Neutral;
}

void AMNPCController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (auto* MCharacter = Cast<AMCharacter>(InPawn))
	{
		// Currently state data isn't used in trees, but it's going to be useful at some point
		MCharacter->OnStateModelUpdatedDelegate.AddUObject(this, &AMNPCController::CopyStateVariablesToBlackboard);
		MCharacter->OnMovedInDelegate.AddLambda([this](const AMOutpostHouse* NewHouse)
		{
			if (NewHouse)
			{
				Blackboard->SetValueAsVector(TEXT("HouseLocation"), NewHouse->GetEntryPoint());

				if (const auto* Outpost = NewHouse->GetOwnerOutpost())
				{
					Blackboard->SetValueAsVector(TEXT("OutpostLocation"), Outpost->GetActorLocation());
				}
			}
		});
		MCharacter->GetCommunicationComponent()->OnInterlocutorChangedDelegate.AddLambda([this](AMCharacter* Interlocutor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Interlocutor is set"));
			Blackboard->SetValueAsObject(TEXT("Interlocutor"), Interlocutor);
		});
	}
}

void AMNPCController::OnUnPossess()
{
	if (auto* MCharacter = Cast<AMCharacter>(GetPawn()))
	{
		MCharacter->OnMovedInDelegate.RemoveAll(this);
		MCharacter->OnStateModelUpdatedDelegate.RemoveAll(this);
	}
	Super::OnUnPossess();
}

void AMNPCController::CopyStateVariablesToBlackboard(const UMStateModelComponent* StateModel)
{
	GetBlackboardComponent()->SetValueAsBool(TEXT("IsCommunicating"), StateModel->GetIsCommunicating());
	// TODO: Copy more when needed
}

void AMNPCController::Embark()
{
	if (const auto ActiveCheckerComponent = Cast<AMCharacter>(GetPawn())->GetIsActiveCheckerComponent())
	{
		ActiveCheckerComponent->SetAlwaysDisabled(true);
		ActiveCheckerComponent->DisableOwner();
	}
	bEmbarked = true;
}

void AMNPCController::OnTurnAround() const
{
	const auto* MyCharacter = Cast<AMCharacter>(GetPawn());
	auto* StateModel = MyCharacter->GetStateModelComponent();
	if (RotationDelta.Yaw >= 3.f)
	{
		StateModel->SetIsTurningRight(true);
		StateModel->SetIsTurningLeft(false);
	} else
	if (RotationDelta.Yaw <= -3.f)
	{
		StateModel->SetIsTurningRight(false);
		StateModel->SetIsTurningLeft(true);
	}
}

void AMNPCController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetPawn() && GetPawn()->GetActorRotation() != LastRotation)
	{
		RotationDelta = GetPawn()->GetActorRotation() - LastRotation;
		OnTurnAround();
		LastRotation = GetPawn()->GetActorRotation();
	}
	else
	{
		const auto* MyCharacter = Cast<AMCharacter>(GetPawn());
		auto* StateModel = MyCharacter->GetStateModelComponent();
		StateModel->SetIsTurningRight(false);
		StateModel->SetIsTurningLeft(false);
	}
}
