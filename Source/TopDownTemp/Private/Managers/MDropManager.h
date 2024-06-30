#pragma once

#include "CoreMinimal.h"
#include "Components/MInventoryComponent.h"
#include "MDropManager.generated.h"

class AMPlayerController;
class UMPickUpBarWidget;
class UMInventoryComponent;
class UUserWidget;
class AMPickableActor;
struct FBundle;

UCLASS(Blueprintable)
class UMDropManager : public UObject //TODO: Rename to ItemManager or whatever..
{
	GENERATED_UCLASS_BODY()

public:

	/** Is called by pickable items to add their inventories to the MPickUpBarWidget for a specific player HUD. */
	void AddInventory(UMInventoryComponent* Inventory, AMPlayerController* PlayerController);

	/** Is called by pickable items to remove their inventories from the MPickUpBarWidget of a specific player HUD. */
	void RemoveInventory(UMInventoryComponent* Inventory, AMPlayerController* PlayerController);

	/** Updates the list of the listed inventories in the MPickUpBarWidget */
	void Update();

	UFUNCTION(BlueprintCallable)
	void SpawnPickableItem(const FItem& Item);

	UFUNCTION()
	void GiveBundleToPlayer(const FBundle& Bundle);

	const TSet<UMInventoryComponent*>& GetInventoriesToRepresent() { return InventoriesToRepresent; }

	static TSubclassOf<UUserWidget> gItemSlotWidgetBPClass;

private:

	virtual void PostInitProperties() override;

	/** Screen side widget with drop available to pick up.\n
	 * We never destroy it but only hide/show. Two main reasons for that:\n
	 * 1. Drag-n-drop might happen after widget is destroyed and won't be resolved;
	 * 2. Avoid overhead of creating and destroying widgets */
	UPROPERTY()
	UMPickUpBarWidget* PickUpBarWidget;

	UPROPERTY(EditDefaultsOnly, Category=UMDropManager)
	TSubclassOf<UUserWidget> PickUpBarWidgetBPClass;

	UPROPERTY(EditDefaultsOnly, Category=UMDropManager)
	TSubclassOf<AMPickableActor> AMPickableItemBPClass;

	UPROPERTY()
	TSet<UMInventoryComponent*> InventoriesToRepresent;

	UPROPERTY(EditDefaultsOnly, Category=MInventoryWidgetSettings)
	TSubclassOf<UUserWidget> ItemSlotWidgetBPClass;
};

