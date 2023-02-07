#include "MAICrowdManager.h"

#include "MCharacter.h"
#include "MMobController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

AMAICrowdManager::AMAICrowdManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ActiveZone = CreateDefaultSubobject<UBoxComponent>(TEXT("Active Zone"));
	ActiveZone->SetBoxExtent(FVector(1500.0f, 1500.0f, 1500.0f));
	ActiveZone->SetGenerateOverlapEvents(false);
	ActiveZone->PrimaryComponentTick.bStartWithTickEnabled = false;
	ActiveZone->PrimaryComponentTick.bCanEverTick = false;
	SetRootComponent(ActiveZone);

	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}


AAIController* AMAICrowdManager::SpawnAIController(const FName& Name, const TSubclassOf<AAIController> ControllerClass, FVector const& Location, FRotator const& Rotation, const FActorSpawnParameters& SpawnParameters)
{
	if (const auto Controller = GetWorld()->SpawnActor<AAIController>(ControllerClass, Location, Rotation, SpawnParameters))
	{
		ControllersMap.Add(Name, Controller);
		return Controller;
	}
	check(false);
	return nullptr;
}

void AMAICrowdManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//TODO: For now the ActiveZone doesn't affect anything.
	//TODO: There might be some logic for disabling some of AIControllers out of the zone
}
