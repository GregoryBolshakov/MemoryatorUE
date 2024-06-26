#pragma once

#include "PaperFlipbookComponent.h"
#include "MRotatableFlipbookComponent.generated.h"

class UPaperFlipbook;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnFlipbookChanged
	, UPaperFlipbook*, Flipbook
	, float, PlaybackPosition
	, float, PlayRate
	, bool, bLoopping
	, bool, bReversePlayback
	, FVector, Scale
	);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpriteChanged);

// TODO: think of using soft pointers
/** An array of pointers to UPaperFlipbook and various settings e.g. play rate, looping, etc. */
USTRUCT()
struct FFlipbooksArray
{
	GENERATED_BODY()
public:
	UPROPERTY(Category=Flipbooks, EditAnywhere, meta=(DisplayThumbnail = "true"))
	TArray<TObjectPtr<UPaperFlipbook>> Flipbooks;

	/** Play rate of the flipbook */
	UPROPERTY(Category=Flipbooks, EditAnywhere)
	float PlayRate = 1.f;

	/** Whether the flipbook should loop when it reaches the end, or stop */
	UPROPERTY(Category=Flipbooks, EditAnywhere)
	uint32 bLooping:1;
};

//TODO: Add a minimum playing time for every flipbook to avoid flickering due to frequent animation changes
/** Component that uses an array of flipbooks to mimic multi-directional actions for a flat body/part.
 *  Not replicated. Actor blueprint needs to store action names in RepNotify FName variables and put them to SetAction() when needed. */
UCLASS(HideCategories=(Sprite), meta=(BlueprintSpawnableComponent))
class TOPDOWNTEMP_API UMRotatableFlipbookComponent : public UPaperFlipbookComponent
{
	GENERATED_UCLASS_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "MRotatableFlipbookComponent")
	FName GetAction() const { return Action; }

	UFUNCTION(BlueprintCallable, Category = "MRotatableFlipbookComponent")
	void SetAction(FName _Action) { Action = _Action; SetFlipbookByRotation(LastValidViewingAngle); }

	/** 
	 * Called to set appropriate flipbook to represent the angular direction. 
	 * @param ViewingAngle is angle between Camera->Actor and ActorsGaze vectors. 0 if actor's gaze matches the camera's
	 */
	void SetFlipbookByRotation(float ViewingAngle);

	UPROPERTY(BlueprintAssignable, Category = "MRotatableFlipbookComponent")
	FOnSpriteChanged OnSpriteChangeDelegate;

	FOnFlipbookChanged OnFlipbookChangedDelegate;

private:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The action currently being played */
	FName Action = FName("Idle");

	UPROPERTY(Category=Flipbooks, EditAnywhere, meta=(DisplayThumbnail = "true"))
	TMap<FName, FFlipbooksArray> FlipbookByAction;

	UPROPERTY()
	UPaperFlipbook* LastValidFlipbook;
	float LastValidPlayRate;
	bool LastValidbLoopping;
	bool LastValidbReversePlayback;
	FVector LastValidScale;
	float LastValidViewingAngle;
};
