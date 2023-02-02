// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperFlipbookComponent.h"
#include "MRotatableFlipbookComponent.generated.h"

class UPaperFlipbook;

/** Structure that provides array placement of UPaperFlipbook ptrs to containers.
 *  Also represents the array in the editor. */
USTRUCT()
struct FFlipbooksArray
{
	GENERATED_BODY()
public:
	UPROPERTY(Category=Sprite, EditAnywhere, meta=(DisplayThumbnail = "true"))
	TArray<TObjectPtr<UPaperFlipbook>> Flipbooks;
};

//TODO: Add a minimum playing time for every flipbook to avoid flickering due to frequent animation changes
UCLASS(meta=(BlueprintSpawnableComponent))
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

private:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** The action currently being played */
	FName Action = FName("Idle");

	UPROPERTY(Category=Sprite, EditAnywhere, meta=(DisplayThumbnail = "true"))
	TMap<FName, FFlipbooksArray> FlipbookByAction;

	/** For inner use of PostEditChangeProperty */
	bool bIsPropertyChanging = false;

	//TODO: Hide the UPaperFlipbookComponent::SourceFlipbook property of the parent 
};
