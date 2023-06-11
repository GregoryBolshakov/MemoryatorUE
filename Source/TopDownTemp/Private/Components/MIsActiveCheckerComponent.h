#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "MIsActiveCheckerComponent.generated.h"

USTRUCT()
struct FDisabledComponentInfo
{
	GENERATED_BODY()
public:
	UPROPERTY()
	UActorComponent* Component;

	bool bCanEverTick = false;

	TOptional<ECollisionEnabled::Type> CollisionType;

	bool bCanCastShadows = false;
};

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="A component that tuns on/off entie object. For example when the object was culled"))
class TOPDOWNTEMP_API UMIsActiveCheckerComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	bool GetIsActive() const { return bIsActive; }

	UPrimitiveComponent* GetPrimitive() const { return CollisionPrimitive; }
	
	void DisableOwner(bool bForce = false);

	void EnableOwner(bool bForce = false);

	bool GetAlwaysEnabled() const { return bAlwaysEnabled; }

	//** Likely to be removed as it is not needed. It finds a UPrimitiveComponent tagged with IsActiveChecker and set it up. Should be called in owner's PostInitializeComponents */
	virtual void SetUpCollisionPrimitive();

private:
	bool bIsActive;

	bool bIsDisabledByForce;

	TOptional<bool> bActorWasHiddenInGame;

	TOptional<bool> bActorHadTickEnabled;

	UPROPERTY()
	TArray<FDisabledComponentInfo> DisabledComponentsData;

	//* Determines the bounds of the object. If it doesnt overlap the World active zone, the object is disabled */
	UPROPERTY(meta = (AllowPrivateAccess = "true"))
	class UPrimitiveComponent* CollisionPrimitive;

	UPROPERTY(Category=Testing, EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool bAlwaysEnabled;
};