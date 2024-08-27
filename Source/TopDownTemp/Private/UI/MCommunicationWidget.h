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

	UFUNCTION(BlueprintCallable)
	void CreateSlots(const UMInventoryComponent* InventoryToOffer, const UMInventoryComponent* InventoryToReward);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void Close();

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
