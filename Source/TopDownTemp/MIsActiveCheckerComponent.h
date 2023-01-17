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
};

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="A component that tuns on/off entie object. For example when the object was culled"))
class TOPDOWNTEMP_API UMIsActiveCheckerComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	bool GetIsActive() const { return bIsActive; }

	UPrimitiveComponent* GetPrimitive() const { return CollisionPrimitive; }
	
	void Disable();

	void Enable();

	//** It will find a UPrimitiveComponent tagged with IsActiveChecker and set it up. Should be called in owner's PostInitializeComponents */
	virtual void SetUpCollisionPrimitive();

private:
	bool bIsActive;

	bool bActorWasHiddenInGame = true;

	bool bActorHadTickEnabled = false;

	UPROPERTY()
	TArray<FDisabledComponentInfo> DisabledComponentsInfo;

	//* Determines the bounds of the object. If it doesnt overlap the World active zone, the object is disabled */
	UPROPERTY(meta = (AllowPrivateAccess = "true"))
	class UPrimitiveComponent* CollisionPrimitive;
};