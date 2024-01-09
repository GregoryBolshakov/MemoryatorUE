#pragma once

#include "CoreMinimal.h"
#include "MCharacter.h"
#include "MCharacter2D.generated.h"

UCLASS(Blueprintable)
class AMCharacter2D : public AMCharacter
{
	GENERATED_UCLASS_BODY()

public:
	virtual void PostInitializeComponents() override;

protected:

	void Tick(float DeltaSeconds) override;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Representation (collection of sprites) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = MCharacterComponents, meta = (AllowPrivateAccess = "true"))
	class UM2DRepresentationComponent* RepresentationComponent;
};

