// Save/Load and server validation logic is exposed to the blueprint

#pragma once

#include "CoreMinimal.h"
#include "Components/MInventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "Managers/SaveManager/MUid.h"
#include "MPlayerController.generated.h"

class UMDropControllerComponent;
class AMCharacter;
class AMActor;
class UCurveFloat;
enum class ERelationType;

UENUM(BlueprintType)
enum class EFloatingNumberType : uint8
{
	Experience = 0,
	Damage,
	Healing
};

UCLASS()
class UCameraOccludedActor : public UObject
{
	GENERATED_BODY()

public:
	FName Name;

	float TargetOpacity = 1.f;
	float TransitionRemainTime = 0.f;
	float LastUpdateTime = 0.f;
	float Distance = 0;
	FTimerHandle OpacityTimerHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	const AMActor* MActor = nullptr;

	bool PendingKill = false;
};

UCLASS()
class AMPlayerController : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable)
	UMDropControllerComponent* GetDropControllerComponent() const { return DropControllerComponent; }

	bool IsMovingByAI() const;

	void StopAIMovement();

	void StartSprintTimer();

	void TurnSprintOn();

	UFUNCTION(BlueprintCallable)
	void TurnSprintOff();

	UFUNCTION()
	void OnExperienceAdded(int Amount);

	UFUNCTION(BlueprintImplementableEvent)
	void MakeFloatingNumber(const FVector& Location, int Value, EFloatingNumberType Type);

	//TODO: Move to a separate class. Likely MDropControllerComponent
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TryDropDraggedOnTheGround();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TryStoreDraggedToAnySlot(FMUid InventoryOwnerActorUid);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TryStoreDraggedToSpecificSlot(FMUid InventoryOwnerActorUid, int SlotNumberInArray);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TryDragItemFromSpecificSlot(FMUid InventoryOwnerActorUid, int SlotNumberInArray, int Quantity);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TrySwapDraggedWithSpecificSlot(FMUid InventoryOwnerActorUid, int SlotNumberInArray);

	UPROPERTY()
	FItem DraggedItem;

protected:
	virtual void AcknowledgePossession(APawn* P) override;

// Occlusion
protected:
	UPROPERTY(EditDefaultsOnly, Category="Camera Occlusion|Occlusion")
	float OccludedOpacity = 0.5f;

	/** Duration of complete transition from opaque to transparent and vice versa. In seconds */
	UPROPERTY(EditDefaultsOnly, Category="Camera Occlusion|Occlusion")
	float OpacityTransitionDuration = 0.7f;

	/** Autonomously updates opacity smoothly for the rest of duration */
	void UpdateOpacity(UCameraOccludedActor* OccludedActor);

	/** How much of the Pawn capsule Radius and Height
	 * should be used for the Line Trace before considering an Actor occluded?
	 * Values too low may make the camera clip through walls.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Occlusion|Occlusion",
		meta=(ClampMin="0.1", ClampMax="10.0"))
	float CapsulePercentageForTrace = 10.0f;

	UPROPERTY(BlueprintReadWrite, Category="Camera Occlusion|Components")
	class USpringArmComponent* ActiveSpringArm;

	UPROPERTY(BlueprintReadWrite, Category="Camera Occlusion|Components")
	class UCameraComponent* ActiveCamera;

	UPROPERTY(BlueprintReadWrite, Category="Camera Occlusion|Components")
	class UCapsuleComponent* ActiveCapsuleComponent;

	/** Responsible for visualising and interacting with drop nearby. Uses only replicated data. Is not authoritative. */
	UPROPERTY()
	UMDropControllerComponent* DropControllerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Occlusion")
	bool IsOcclusionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Occlusion|Occlusion")
	bool DebugLineTraces = false;

private:
	UPROPERTY()
	TMap<FName, UCameraOccludedActor*> OccludedActors;

	void HideOccludedActor(const AMActor* MActor, float Distance);
	void OnHideOccludedActor(UCameraOccludedActor* OccludedActor);
	void ShowOccludedActor(UCameraOccludedActor* OccludedActor);
	void OnShowOccludedActor(UCameraOccludedActor* OccludedActor);

	__forceinline bool ShouldCheckCameraOcclusion() const
	{
		return IsOcclusionEnabled && ActiveCamera && ActiveCapsuleComponent;
	}

public:
	UFUNCTION(BlueprintCallable)
	void SyncOccludedActors();

	/** Here is cached the pawn for a connecting player if the controller has not begun play. */
	UPROPERTY()
	APawn* DeferredPawnToPossess;

	/** Flag bit index. Multiplayer limit is 32 players. Each block knows whether a certain player is observing it.\n
	* Each player is given a unique flag bit position when entering the game. */
	UPROPERTY()
	uint8 ObserverIndex = -1;

	// Other
protected:
	virtual void BeginPlay() override;

private:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;
	bool bIsTurningAround = false;

	UPROPERTY()
	class UPathFollowingComponent* PathFollowingComponent;

	UPROPERTY(EditDefaultsOnly)
	class UMConsoleCommandsManager* ConsoleCommandsManager;

	// Begin PlayerController interface
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

	void MoveRight(float Value);
	void MoveForward(float Value);

	void TurnAround(float Value);

	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();

	/** Navigate player to the current touch location. */
	void MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location);

	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	//TODO: Figure out of we're gonna need this at all
	/** Input handlers for SetDestination action. */
	//void OnSetDestinationPressed();
	//void OnSetDestinationReleased();

	void OnToggleTurnAroundPressed();
	void OnToggleTurnAroundReleased();

	void OnToggleFightPressed();

	void OnLeftMouseClick();

	virtual bool ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor) override;
};
