#pragma once

#include "CoreMinimal.h"
#include "MInventoryComponent.h"
#include "MDropManager.generated.h"

class UMPickUpBarWidget;
class UMInventoryComponent;
class UUserWidget;
class AMPickableItem;

UCLASS(Blueprintable)
class UMDropManager : public UObject
{
	GENERATED_BODY()

public:

	void AddInventory(UMInventoryComponent* Inventory);

	void RemoveInventory(UMInventoryComponent* Inventory);

	void Update();

	UFUNCTION(BlueprintCallable)
	bool OnDraggedItemDropped(const FItem& Item);

private:

	UPROPERTY()
	UMPickUpBarWidget* PickUpBarWidget;

	UPROPERTY(EditDefaultsOnly, Category=UMDropManager)
	TSubclassOf<UUserWidget> PickUpBarWidgetBPClass;

	UPROPERTY(EditDefaultsOnly, Category=UMDropManager)
	TSubclassOf<AMPickableItem> AMPickableItemBPClass;

	UPROPERTY()
	TSet<UMInventoryComponent*> InventoriesToRepresent;
};

