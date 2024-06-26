#pragma once

#include "CoreMinimal.h"
#include "MCharacter.h"
#include "MMemoryator.generated.h"

UCLASS(Blueprintable)
class AMMemoryator : public AMCharacter
{
	GENERATED_UCLASS_BODY()

public:
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.0f, bool bForce = false) override;

	virtual void Tick(float DeltaSeconds) override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns CursorToWorld subobject **/
	FORCEINLINE class USceneComponent* GetCursorToWorld() { return CursorToWorld; }

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void OnRep_PlayerState() override; 	// Client only

	// TODO: When ASC is moved to Player State: Call from both SetupPlayerInputComponent and OnRep_PlayerState
	// because of a potential race condition
	void BindASCInput();

// Interaction with other mobs
protected:
	void SetDynamicActorsNearby();

	void UpdateClosestEnemy();

	UPROPERTY()
	TMap<FName, AActor*> EnemiesNearby;

	UPROPERTY()
	AActor* ClosestEnemy;

	FTimerHandle ActorsNearbyUpdateTimerHandle;

private:
	void HandleCursor() const;

	void HandleMovementState();

	virtual void BeginPlay() override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** A decal that projects to the cursor location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* CursorToWorld;

	bool bASCInputBound = false;
};

