// Copyright Epic Games, Inc. All Rights Reserved.

#include "MRotatableFlipbookComponent.h"
#include "PaperFlipbookComponent.h"

UMRotatableFlipbookComponent::UMRotatableFlipbookComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMRotatableFlipbookComponent::SetFlipbookByRotation(float ViewingAngle)
{
	if (const auto FlipbookArray = FlipbookByAction.Find(Action))
	{
		const auto FlipbooksCount = FlipbookArray->Flipbooks.Num();
		if (FlipbooksCount == 0)
		{
			SetFlipbook(nullptr);
			return;
		}

		const auto sign = ViewingAngle >= 0.f ? 1.f : -1.f;
		// The angle of object's gaze. 0 means match with camera vector,
		// 90 = watches to the right, 180 or -180 = face the camera, -90 = watches to the left
		ViewingAngle = abs(ViewingAngle);

		// Determine to which sector of the coordinate circle ViewingAngle belongs
		int FlipbookIndex;
		if (FlipbooksCount <= 2)
		{
			const int AngleSegmentValue = 180.f / FlipbooksCount;
			FlipbookIndex = StaticCast<int>(ViewingAngle / AngleSegmentValue);
		}
		else
		{
			const int AngleSegmentValue = 180.f / (FlipbooksCount - 1);
			FlipbookIndex = StaticCast<int>((ViewingAngle + AngleSegmentValue / 2) / AngleSegmentValue);
		}

		if (FlipbookIndex >= FlipbooksCount)
		{
			SetFlipbook(nullptr);
			return;
		}

		if (SourceFlipbook != FlipbookArray->Flipbooks[FlipbookIndex])
		{
			const auto PlaybackPosition = GetPlaybackPosition();
			SetFlipbook(FlipbookArray->Flipbooks[FlipbookIndex]);
			SetPlaybackPosition(PlaybackPosition, false);

			SetPlayRate(FlipbookArray->PlayRate);
			SetLooping(FlipbookArray->bLooping);
			bReversePlayback = FlipbookArray->bReversePlayback;
		}

		// Mirror the flipbook if needed
		FVector Scale = GetRelativeScale3D();
		if (sign == -1.f && FlipbookIndex != 0 && FlipbookIndex != FlipbooksCount - 1)
		{
			Scale.X = abs(Scale.X);
		}
		else
		{
			Scale.X = -abs(Scale.X);
		}
		SetRelativeScale3D(Scale);
	}
	else
	{
		SetFlipbook(nullptr);
	}
}

void UMRotatableFlipbookComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	const int32 LastCachedFrame = CachedFrameIndex;

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (CachedFrameIndex != LastCachedFrame)
	{
		OnSpriteChangeDelegate.Broadcast();
	}

#if WITH_EDITOR
	if (!GIsPlayInEditorWorld)
	{
		// Update displayed flipbook using the first valid one
		if (FlipbookByAction.Num() != 0)
		{
			const auto FirstValidAction = TMap<FName, FFlipbooksArray>::TIterator(FlipbookByAction).Value();
			if (FirstValidAction.Flipbooks.Num() > 0 && SourceFlipbook != FirstValidAction.Flipbooks[0])
			{
				SetFlipbook(FirstValidAction.Flipbooks[0]);
				SetPlayRate(FirstValidAction.PlayRate);
				SetLooping(FirstValidAction.bLooping);
				bReversePlayback = FirstValidAction.bReversePlayback;
			}
		}
	}
#endif
}
