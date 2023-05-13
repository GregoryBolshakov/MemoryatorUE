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

	/** Is called by pickable items to add their inventories to the MPickUpBarWidget */
	void AddInventory(UMInventoryComponent* Inventory);

	/** Is called by pickable items to remove their inventories to the MPickUpBarWidget */
	void RemoveInventory(UMInventoryComponent* Inventory);

	/** Updates the list of the listed inventories in the MPickUpBarWidget */
	void Update();

	UFUNCTION(BlueprintCallable)
	void SpawnPickableItem(const FItem& Item);

	const TSet<UMInventoryComponent*>& GetInventoriesToRepresent() { return InventoriesToRepresent; }

	static TSubclassOf<UUserWidget> gItemSlotWidgetBPClass;

private:

	virtual void PostInitProperties() override;

	UPROPERTY()
	UMPickUpBarWidget* PickUpBarWidget;

	UPROPERTY(EditDefaultsOnly, Category=UMDropManager)
	TSubclassOf<UUserWidget> PickUpBarWidgetBPClass;

	UPROPERTY(EditDefaultsOnly, Category=UMDropManager)
	TSubclassOf<AMPickableItem> AMPickableItemBPClass;

	UPROPERTY()
	TSet<UMInventoryComponent*> InventoriesToRepresent;

	UPROPERTY(EditDefaultsOnly, Category=MInventoryWidgetSettings)
	TSubclassOf<UUserWidget> ItemSlotWidgetBPClass;
};

