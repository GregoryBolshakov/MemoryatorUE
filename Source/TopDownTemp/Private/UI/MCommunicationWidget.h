#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MCommunicationWidget.generated.h"

class UButton;
class AMPickableActor;
class UWrapBox;

UCLASS()
class TOPDOWNTEMP_API UMCommunicationWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void Close();

	UFUNCTION(BlueprintCallable)
	void CreateItemSlotWidgets();

	UFUNCTION()
	void ReCreateRewardItemSlotWidgets();

protected:

	UPROPERTY(EditDefaultsOnly, Category=MInventoryWidgetSettings)
	TSubclassOf<UUserWidget> ItemSlotWidgetBPClass;

	UPROPERTY(BlueprintReadWrite, Category=MInventoryWidgetSettings)
	UWrapBox* pMyItemSlotsWrapBox;

	UPROPERTY(BlueprintReadWrite, Category=MInventoryWidgetSettings)
	UWrapBox* pTheirItemSlotsWrapBox;

	UPROPERTY(BlueprintReadWrite, Category=MInventoryWidgetSettings)
	UWrapBox* pRewardItemSlotsWrapBox;

	UPROPERTY(BlueprintReadWrite, Category=MInventoryWidgetSettings)
	UButton* pTakeAllButton;
};
