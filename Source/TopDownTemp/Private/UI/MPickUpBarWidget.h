#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/MInventoryComponent.h"
#include "MPickUpBarWidget.generated.h"

class UWrapBox;

UCLASS()
class TOPDOWNTEMP_API UMPickUpBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void CreateSlots(const TSet<UMInventoryComponent*>& InventoriesToRepresent);

	UFUNCTION(BlueprintImplementableEvent)
	void Show();

	UFUNCTION(BlueprintImplementableEvent)
	void Hide();

protected:

	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, Category=MPickUpBarWidget, meta=(AllowPrivateAccess=true))
	TSubclassOf<UUserWidget> ItemSlotWidgetBPClass;

	UPROPERTY(BlueprintReadWrite, Category=MPickUpBarWidget)
	UWrapBox* pItemSlotsWrapBox;


};