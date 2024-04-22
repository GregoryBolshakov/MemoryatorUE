#include "MGameMode.h"
#include "Controllers/MPlayerController.h"
#include "Characters/MCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Managers/MBlockGenerator.h"
#include "Managers/MCommunicationManager.h"
#include "Managers/MDropManager.h"
#include "Managers/MExperienceManager.h"
#include "Managers/MMetadataManager.h"
#include "Managers/MReputationManager.h"
#include "Managers/MWorldGenerator.h"
#include "Managers/RoadManager/MRoadManager.h"
#include "Managers/SaveManager/MSaveManager.h"
#include "UObject/ConstructorHelpers.h"

AMGameMode::AMGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = AMPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Memoryator/Blueprints/BP_Memoryator"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AMGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (!WorldGenerator)
	{
		ConnectionQueue.Enqueue(NewPlayer); // Connection received before world systems are initialized, will be processed later
	}
	else
	{
		WorldGenerator->ProcessConnectingPlayer(NewPlayer);
	}
}

void AMGameMode::GoOffline()
{
}

void AMGameMode::BeginPlay()
{
	Super::BeginPlay();
	InitializeManagers();
	while (!ConnectionQueue.IsEmpty())
	{
		APlayerController* PlayerController;
		ConnectionQueue.Dequeue(PlayerController);
		WorldGenerator->ProcessConnectingPlayer(PlayerController); // Process deferred connections since all systems are ready
	}
}

void AMGameMode::InitializeManagers()
{
	WorldGenerator = GetWorld()->SpawnActor<AMWorldGenerator>(WorldGeneratorBPClass, FVector::ZeroVector, FRotator::ZeroRotator, {});
	check(WorldGenerator);

	MetadataManager = NewObject<UMMetadataManager>(GetOuter(), UMMetadataManager::StaticClass(), TEXT("MetadataManager"));
	check(MetadataManager);
	MetadataManager->Initialize(WorldGenerator->GetBlockGenerator()->GetDefaultGraph());

	DropManager = DropManagerBPClass ? NewObject<UMDropManager>(GetOuter(), DropManagerBPClass, TEXT("DropManager")) : nullptr;
	check(DropManager);

	ReputationManager = ReputationManagerBPClass ? NewObject<UMReputationManager>(GetOuter(), ReputationManagerBPClass, TEXT("ReputationManager")) : nullptr;
	check(ReputationManager);
	ReputationManager->Initialize({{EFaction::Humans, {100, 10}}, {EFaction::Nightmares, {5, 4}}, {EFaction::Witches, {1, 0}}}); // temporarily set manually

	ExperienceManager = ExperienceManagerBPClass ? NewObject<UMExperienceManager>(GetOuter(), ExperienceManagerBPClass, TEXT("ExperienceManager")) : nullptr;
	check(ExperienceManager);

	SaveManager = SaveManagerBPClass ? NewObject<UMSaveManager>(GetOuter(), SaveManagerBPClass, TEXT("SaveManager")) : nullptr;
	check(SaveManager);
	SaveManager->LoadFromMemory();
	// TODO: Saving blocks having saved players which are not playing at the moment looks dangerous as it will override players. Think of a way to keep them
	SaveManager->SetUpAutoSaves(WorldGenerator);

	// Spawn the Communication Manager
	CommunicationManager = CommunicationManagerBPClass ? GetWorld()->SpawnActor<AMCommunicationManager>(CommunicationManagerBPClass) : nullptr;
	check(CommunicationManager);

	RoadManager = NewObject<UMRoadManager>(GetOuter(), RoadManagerBPClass, TEXT("RoadManager"));
	check(RoadManager);
	RoadManager->Initialize(WorldGenerator);
	WorldGenerator->SetupInputComponent();
}
