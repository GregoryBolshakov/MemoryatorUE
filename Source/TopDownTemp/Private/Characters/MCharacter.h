#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MCharacterSpecificTypes.h"
#include "MCharacter.generated.h"

class UM2DRepresentationComponent;
class UMStateModelComponent;
class UMStatsModelComponent;
class UIsActiveCheckerComponent;
class UMIsActiveCheckerComponent;
class UMInventoryComponent;
class UAbilitySystemComponent;
class UMAbilitySystemComponent;
class UMGameplayAbility;
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

	FVector GetForcedGazeVector() const { return ForcedGazeVector; }

	UMStateModelComponent* GetStateModelComponent() const { return StateModelComponent; }

	UMStatsModelComponent* GetStatsModelComponent() const { return StatsModelComponent; }

	UMIsActiveCheckerComponent* GetIsActiveCheckerComponent() const { return IsActiveCheckerComponent; }

	UMInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UMAttackPuddleComponent* GetAttackPuddleComponent() const { return AttackPuddleComponent; }

	UMCommunicationComponent* GetCommunicationComponent() const { return CommunicationComponent; }

	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	FName GetSpeciesName() const { return SpeciesName; }

	FORCEINLINE const TMap<TSubclassOf<APawn>, ERelationType>& GetRelationshipMap() const { return RelationshipMap; }

	FVector GetLastNonZeroVelocity() const { return LastNonZeroVelocity; }

	AMOutpostHouse* GetHouse() const { return House; }

	void InitialiseInventory(const TArray<struct FItem>& IN_Items) const;

	UFUNCTION(BlueprintCallable)
	void SetForcedGazeVector(FVector Vector) { ForcedGazeVector = Vector; }

	void OnMovedIn(AMOutpostHouse* NewHouse) { House = NewHouse; }

	void OnMovedOut() { House = nullptr; }

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	virtual void PostInitializeComponents() override;

	/** Start from the base (FActorSaveData -> might add more in between -> FMCharacterSaveData).\n
	 * Must always call Super::GetSaveData(). */
	virtual FMCharacterSaveData GetSaveData() const;

	/** Loading actor: Actor has just constructed but not finalized (Components are not available).*/
	virtual void BeginLoadFromSD(const FMCharacterSaveData& MCharacterSD);
	// TODO: We might need EndLoadFromSD() but so far there's no use cases

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void UpdateAnimation(); // TODO: Move to protected;

protected:
	void UpdateLastNonZeroDirection();

	virtual void BeginPlay() override;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintNativeEvent)
	void OnEnabled();

	UFUNCTION(BlueprintNativeEvent)
	void OnDisabled();

	virtual void PossessedBy(AController* NewController) override;

	/** Representation (collection of sprites) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UM2DRepresentationComponent* FaceCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UMStateModelComponent* StateModelComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UMStatsModelComponent* StatsModelComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UMAbilitySystemComponent* AbilitySystemComponent;

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

	/** Stores relationship with other pawns. Neutral if not listed */
	UPROPERTY(EditAnywhere, Category = BehaviorParameters, meta=(AllowPrivateAccess = true))
	TMap<TSubclassOf<APawn>, ERelationType> RelationshipMap;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector LastNonZeroVelocity = FVector(1.f, 0.f, 0.f);

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	FVector ForcedGazeVector;

	UPROPERTY(BlueprintAssignable, Category = "MRotatableFlipbookComponent")
	FOnReverseMovementStarted OnReverseMovementStartedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "MRotatableFlipbookComponent")
	FOnReverseMovementStopped OnReverseMovementStoppedDelegate;

	//TODO: create a separate entity to store this. Now it needs to be set for each ancestor (it's bad).
	UPROPERTY(EditDefaultsOnly, Category=MBuffManagerComponent)
	TSubclassOf<UUserWidget> BuffBarWidgetBPClass;

	UPROPERTY()
	AMOutpostHouse* House;

// Ability system
public:
	// Switch on AbilityID to return individual ability levels.
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	virtual int32 GetAbilityLevel(EMAbilityInputID AbilityID) const;

// Ability system
protected:
	/** Grant abilities on the Server. The Ability Specs will be replicated to the owning client. */
	virtual void AddCharacterAbilities();

	// TODO: virtual void RemoveCharacterAbilities();
	// TODO: Removes all CharacterAbilities. Can only be called by the Server. Removing on the Server will remove from Client too.

	// Default abilities for this Character. They will be removed on Character death and re-given if Character respawns.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UMGameplayAbility>> CharacterAbilities;
};

