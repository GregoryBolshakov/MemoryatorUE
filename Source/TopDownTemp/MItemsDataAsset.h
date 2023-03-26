// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MItemsDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FItemData
{
	GENERATED_BODY()

	UPROPERTY(Category=ItemData, EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail="true"))
	UTexture2D* IconTexture;

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
