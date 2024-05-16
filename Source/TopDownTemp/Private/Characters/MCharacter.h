#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MCharacter.generated.h"

class UM2DRepresentationComponent;
class UMStateModelComponent;
class UMIsActiveCheckerComponent;
class UMInventoryComponent;
class UMCommunicationComponent;
class UMAttackPuddleComponent;
class AMOutpostHouse;
struct FMCharacterSaveData;
class UMBuffManagerComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReverseMovementStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReverseMovementStopped);

UCLASS(Blueprintable)
class AMCharacter : public ACharacter
{
	GENERATED_UCLASS_BODY()

public:

	/** Use AMWorldGenerator::RemoveFromGrid instead */
	//bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true ) = delete;

	/** Never destroy objects within the grid using AActor::Destroy. Identical naming is used to minimize calls to the wrong Destroy().
	 *  But we are still not immune from incorrect calls if the pointer to a MActor or MCharacter is of type AActor */
	bool Destroy(bool bNetForce = false, bool bShouldModifyLevel = true);

	float GetRadius() const;

	float GetSightRange() const { return SightRange; }

	float GetFightRangePlusMyRadius() const { return FightRange + GetRadius(); }

	float GetForgetEnemyRange() const { return ForgetEnemyRange; }

	float GetHealth() const { return Health; }

	float GetRetreatRange() const { return RetreatRange; }

	float GetWalkSpeed() const { return WalkSpeed; }

	float GetSprintSpeed() const { return SprintSpeed; }

	float GetStrength() const { return Strength; }

	bool GetCanRetreat() const { return bCanRetreat; }

	float GetTimeBeforeSprint() const { return TimeBeforeSprint; }

	FVector GetForcedGazeVector() const { return ForcedGazeVector; }

	UMStateModelComponent* GetStateModelComponent() const { return StateModelComponent; }

	UMIsActiveCheckerComponent* GetIsActiveCheckerComponent() const { return IsActiveCheckerComponent; }

	UMInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UMAttackPuddleComponent* GetAttackPuddleComponent() const { return AttackPuddleComponent; }

	UMCommunicationComponent* GetCommunicationComponent() const { return CommunicationComponent; }

	FVector GetLastNonZeroVelocity() const { return LastNonZeroVelocity; }

	FName GetSpeciesName() const { return SpeciesName; }

	AMOutpostHouse* GetHouse() const { return House; }

	void InitialiseInventory(const TArray<struct FItem>& IN_Items) const;

	void SetForcedGazeVector(FVector Vector) { ForcedGazeVector = Vector; }

	void OnMovedIn(AMOutpostHouse* NewHouse) { House = NewHouse; }

	void OnMovedOut() { House = nullptr; }

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	virtual void PostInitializeComponents() override;

	/** Loading actor: Actor has just constructed but not finalized (Components are not available).*/
	virtual void BeginLoadFromSD(const FMCharacterSaveData& MCharacterSD);
	// TODO: We might need EndLoadFromSD() but so far there's no use cases

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void UpdateAnimation();

	void UpdateLastNonZeroDirection();

	virtual void BeginPlay() override;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintNativeEvent)
	void OnEnabled();

	UFUNCTION(BlueprintNativeEvent)
	void OnDisabled();

	/** Representation (collection of sprites) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UM2DRepresentationComponent* OptionalRepresentationComponent; //TODO: Remove this. Temp workaround for legacy 2D actors

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UMStateModelComponent* StateModelComponent;
	/** Triggers replication by flipping value. */
	UPROPERTY(ReplicatedUsing=OnRep_StateModelReplicationTrigger)
	bool StateModelReplicationTrigger = false;
	// We have to trigger state model replication from the owner level (MCharacter/MActor)
	// because components are less reliable and sometimes are ignored for replication.
	// Initially this was tracked and called from UMStateModelComponent.
	UFUNCTION()
	void OnRep_StateModelReplicationTrigger();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UMInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UMCommunicationComponent* CommunicationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMIsActiveCheckerComponent* IsActiveCheckerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UMBuffBarComponent* BuffBarComponent;

	//TODO: Consider creating separate manager for all underfoot UI
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UMAttackPuddleComponent* AttackPuddleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UPaperSpriteComponent* PerimeterOutlineComponent;

	UPROPERTY()
	FName SpeciesName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector LastNonZeroVelocity = FVector(1.f, 0.f, 0.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector ForcedGazeVector;

	UPROPERTY(BlueprintAssignable, Category = "MRotatableFlipbookComponent", meta=(AllowPrivateAccess=true))
	FOnReverseMovementStarted OnReverseMovementStartedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "MRotatableFlipbookComponent", meta=(AllowPrivateAccess=true))
	FOnReverseMovementStopped OnReverseMovementStoppedDelegate;

	//TODO: create a separate entity to store this. Now it needs to be set for each ancestor (it's bad).
	UPROPERTY(EditDefaultsOnly, Category=MBuffManagerComponent)
	TSubclassOf<UUserWidget> BuffBarWidgetBPClass;

	UPROPERTY()
	AMOutpostHouse* House;

	//TODO: Create another model for stats. It also will be storing their original values e.g. MaxHealth, DefaultSightRange, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float Health;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float SightRange = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float FightRange = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float ForgetEnemyRange = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float Strength = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float RetreatRange = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	bool bCanRetreat = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float MeleeSpread = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float WalkSpeed = 185.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float SprintSpeed = 260.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Perks, meta = (AllowPrivateAccess = "true"))
	float TimeBeforeSprint = 1.f;
};

