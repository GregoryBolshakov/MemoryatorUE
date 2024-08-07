#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MCharacterSpecificTypes.h"
#include "Managers/SaveManager/MUid.h"
#include "AI/MTeamIds.h"
#include "GenericTeamAgentInterface.h"
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
class UAnimMontage;
class AMOutpostHouse;
struct FMCharacterSaveData;
class UMBuffManagerComponent;

/** Called when the character moves into a house (or any other kind of place to live) */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMovedInDelegate, const AMOutpostHouse* NewHouse);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStateModelUpdated, const UMStateModelComponent* StateModel);

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

	UFUNCTION(BlueprintCallable)
	FMUid GetUid() const { return Uid; }

	FName GetSpeciesName() const { return SpeciesName; }

	/** Should be used only by controllers in overridden UGenericTeamAgentInterface functions.
	 * Otherwise, use FGenericTeamId::GetAttitude() */
	const TMap<TSubclassOf<APawn>, TEnumAsByte<ETeamAttitude::Type>>& GetCustomAttitudes() const { return CustomAttitudes; }

	/** Should be used only by controllers in overridden UGenericTeamAgentInterface::GetGenericTeamId() */
	FGenericTeamId GetTeamID() const { return GetTeamIdByEnum(TeamID); }

	FVector GetLastNonZeroVelocity() const { return LastNonZeroVelocity; }

	UFUNCTION(BlueprintCallable)
	AMOutpostHouse* GetHouse() const { return House; }

	void SetUid(const FMUid& _Uid) { Uid = _Uid; }

	UFUNCTION(BlueprintCallable)
	void InitialiseInventory(const TArray<struct FItem>& IN_Items);

	UFUNCTION(BlueprintCallable)
	void SetForcedGazeVector(FVector Vector) { ForcedGazeVector = Vector; }

	void OnMovedIn(AMOutpostHouse* NewHouse);

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

	/** Outdated function used for paper sprites */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Animation")
	void UpdateAnimation(); // TODO: Move to protected;

	UFUNCTION(BlueprintCallable)
	void PlayAnimMontageMultiplayer(UAnimMontage* AnimMontage);

	FOnMovedInDelegate OnMovedInDelegate;
	FOnStateModelUpdated OnStateModelUpdatedDelegate;

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

	// I don't like having it both here and in the metadata. But client needs it to do requests
	UPROPERTY(Replicated)
	FMUid Uid;

	UPROPERTY()
	FName SpeciesName;

	/** We store TeamId on actors instead of controllers due to the overall design of the game.\n
	 * Normally, CustomAttitudes map has higher priority. TeamID is used only when there's no mapping. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Affiliation)
	EMTeamID TeamID;

	/** Stores custom attitudes to other pawns. Neutral if not listed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Affiliation)
	TMap<TSubclassOf<APawn>, TEnumAsByte<ETeamAttitude::Type>> CustomAttitudes;

	/** Currently of no use. But keep it for later. */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector LastNonZeroVelocity = FVector(1.f, 0.f, 0.f);

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	FVector ForcedGazeVector;

	/** The actual gaze (rotation). If Forced one isn't zero, use it; Otherwise if current Velocity isn't zero, use it; Otherwise remain the same. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector CurrentGazeVector;

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

// Montages
private:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayMontage(UAnimMontage* AnimMontage);

	UFUNCTION(Server, Reliable)
	void Server_PlayMontage(UAnimMontage* AnimMontage);

	void PlayMontageImpl(UAnimMontage* AnimMontage);
};

