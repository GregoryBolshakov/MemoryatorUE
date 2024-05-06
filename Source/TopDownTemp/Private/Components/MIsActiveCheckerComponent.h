#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "MIsActiveCheckerComponent.generated.h"

DECLARE_DELEGATE(FOnDisabled);
DECLARE_DELEGATE(FOnEnabled);

USTRUCT()
struct FDisabledComponentInfo
{
	GENERATED_BODY()
public:
	UPROPERTY()
	UActorComponent* Component;

	bool bCanEverTick = false;

	TOptional<ECollisionEnabled::Type> CollisionType;
};

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="A component that tuns on/off entie object. For example when the object was culled"))
class TOPDOWNTEMP_API UMIsActiveCheckerComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	bool GetIsActive() const { return bIsOwnerActive; }

	class UPrimitiveComponent* GetPrimitive() const { return CollisionPrimitive; }

	void DisableOwner();

	void EnableOwner();

	void SetAlwaysDisabled(bool Value) { bAlwaysDisabled = Value; }

	bool GetAlwaysEnabled() const { return bAlwaysEnabled; }

	bool GetAlwaysDisabled() const { return bAlwaysDisabled; }

	bool GetPreserveBlockConstancy() const { return bPreserveBlockConstancy; }

	//** Likely to be removed as it is not needed. It finds a UPrimitiveComponent tagged with IsActiveChecker and set it up. Should be called in owner's PostInitializeComponents */
	virtual void SetUpCollisionPrimitive();

	FOnDisabled OnDisabledDelegate;
	FOnEnabled OnEnabledDelegate;

protected:
	/** Determines the bounds of the object. If it doesnt overlap the World active zone, the object is disabled */
	UPROPERTY()
	UPrimitiveComponent* CollisionPrimitive = nullptr;

	/** The owner never stops ticking and remains visible even beyond the frustum. Any engine optimizations/culls still applicable */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bAlwaysEnabled = false;

	/** The owner stops ticking, being visible, interactable or replicated */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bAlwaysDisabled = false;

	/** If true, the block on which the owner is located is not regenerated by the game feature */
	UPROPERTY(Category=Testing, VisibleAnywhere, BlueprintReadWrite)
	bool bPreserveBlockConstancy = false;

private: // Saved data
	UPROPERTY(VisibleAnywhere)
	bool bIsOwnerActive = true;

	TOptional<bool> bActorWasHiddenInGame;

	TOptional<bool> bActorHadTickEnabled;

	TOptional<bool> bWasActorReplicated;

	UPROPERTY()
	TArray<FDisabledComponentInfo> DisabledComponentsData;
};
