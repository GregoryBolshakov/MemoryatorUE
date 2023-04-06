// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PaperSprite.h"
#include "MItemsDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FItemData
{
	GENERATED_BODY()

	/** For the UI */
	UPROPERTY(Category=ItemData, EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail="true"))
	UTexture2D* IconTexture;

	/** For the in-world usage */
	UPROPERTY(Category=ItemData, EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail="true"))
	UPaperSprite* IconSprite;

	UPROPERTY(Category=ItemData, EditAnywhere, BlueprintReadOnly)
	int MaxStack;
};

UCLASS(BlueprintType)
class TOPDOWNTEMP_API UMItemsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(Category=ItemsData, EditAnywhere, BlueprintReadOnly)
	TArray<FItemData> ItemsData;
};
