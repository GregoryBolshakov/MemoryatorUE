#pragma once

#include "AIController.h"
#include "MAICrowdManager.generated.h"

//~=============================================================================
/**
 * Manages all NPCs in the game. Can turn them on/off, change the difficulty, etc.
 */
UCLASS(Blueprintable)
class TOPDOWNTEMP_API AMAICrowdManager : public AActor //TODO: Maybe remove this, now is useless
{
	GENERATED_UCLASS_BODY()

public:

	/** Spawn the pawn controller
	* @param	Name			Name of possessed APawn
	* @param	ControllerClass	Class of the controller to spawn
	* @param	Location		Location of possessed APawn
	* @param	Rotation		Rotation of possessed APawn
	* @param	SpawnParameters	SpawnParameters of possessed APawn
	*/
	AAIController* SpawnAIController(const FName& Name, const TSubclassOf<AAIController> ControllerClass, FVector const& Location, FRotator const& Rotation, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());

private:

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY()
	TMap<FName, AAIController*> ControllersMap;

	/** The box indicating the bounds of the interaction area of the world. */ 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger Box", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* ActiveZone; //TODO: Implement the logic for it. Now it affects nothing.

};