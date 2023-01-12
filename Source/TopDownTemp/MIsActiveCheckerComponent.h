#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "MIsActiveCheckerComponent.generated.h"

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="A component that tuns on/off entie object. For example when the object was culled"))
class TOPDOWNTEMP_API UMIsActiveCheckerComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	bool GetIsActive() const { return bIsActive; }

	void Disable();

	void Enable();

	virtual void BeginPlay() override;

private:
	bool bIsActive;

	UPROPERTY(meta = (AllowPrivateAccess = "true"))
	class UShapeComponent* Scope;
};