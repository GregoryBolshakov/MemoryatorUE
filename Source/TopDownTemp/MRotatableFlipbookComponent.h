// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperFlipbookComponent.h"
#include "MRotatableFlipbookComponent.generated.h"

class UPaperFlipbook;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpriteChange);

/** Structure that provides array placement of UPaperFlipbook ptrs to containers.
 *  Also represents the array in the editor. */
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

	/** If playback should move the current position backwards instead of forwards */
	UPROPERTY(Category=Flipbooks, EditAnywhere)
	uint32 bReversePlayback:1;
};

//TODO: Add a minimum playing time for every flipbook to avoid flickering due to frequent animation changes
UCLASS(HideCategories=(Sprite), meta=(BlueprintSpawnableComponent))
class TOPDOWNTEMP_API UMRotatableFlipbookComponent : public UPaperFlipbookComponent
{
	GENERATED_UCLASS_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Rendering")
	void SetAction(const FName& _Action) { Action = _Action; }

	/** 
	 * Called to set appropriate flipbook to represent the angular direction. 
	 * @param ViewingAngle is angle between Camera->Actor and ActorsGaze vectors. 0 if actor's gaze matches the camera's
	 */
	void SetFlipbookByRotation(float ViewingAngle);

	UPROPERTY(EditAnywhere, BlueprintAssignable)
	FOnSpriteChange OnSpriteChangeDelegate;

private:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The action currently being played */
	FName Action = FName("Idle");

	UPROPERTY(Category=Flipbooks, EditAnywhere, meta=(DisplayThumbnail = "true"))
	TMap<FName, FFlipbooksArray> FlipbookByAction;
};
