#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MInventorySlotWidget.generated.h"

class UMInventoryComponent;

UCLASS(Blueprintable, BlueprintType)
class TOPDOWNTEMP_API UMInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetNumberInArray(int IN_NumberInArray) { NumberInArray = IN_NumberInArray; }

	void SetOwnerInventory(UMInventoryComponent* IN_OwnerInventory) { OwnerInventory = IN_OwnerInventory; }

	UFUNCTION(BlueprintImplementableEvent)
	void OnChangedData(int NewItemID, int NewQuantity);

protected:

	UPROPERTY(BlueprintReadOnly, Category=UMInventorySlotWidget, meta=(AllowPrivateAccess=true))
	int NumberInArray;

	UPROPERTY(BlueprintReadOnly, Category=UMInventorySlotWidget, meta=(AllowPrivateAccess=true))
	UMInventoryComponent* OwnerInventory;
};

