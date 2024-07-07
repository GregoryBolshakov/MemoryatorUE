#pragma once

#include "CoreMinimal.h"
#include "Components/MInventoryComponent.h"
#include "MDropManager.generated.h"

class UMInventoryWidget;
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

	/** Spawns a AMPickableActor near the Owner with one item in the inventory */
	UFUNCTION(BlueprintCallable)
	void SpawnPickableItem(const AActor* Owner, const FItem& Item);

	UFUNCTION()
	void GiveBundleToPlayer(const FBundle& Bundle);

	const TSet<UMInventoryComponent*>& GetInventoriesToRepresent() { return InventoriesToRepresent; }

	static TSubclassOf<UUserWidget> gItemSlotWidgetBPClass;
	static TSubclassOf<UMPickUpBarWidget> gPickUpBarWidgetBPClass;
	static TSubclassOf<UMInventoryWidget> gInventoryWidgetBPClass;

private:

	virtual void PostInitProperties() override;

	/** Screen side widget with drop available to pick up.\n
	 * We never destroy it but only hide/show. Two main reasons for that:\n
	 * 1. Drag-n-drop might happen after widget is destroyed and won't be resolved;
	 * 2. Avoid overhead of creating and destroying widgets */
	UPROPERTY()
	UMPickUpBarWidget* PickUpBarWidget;

	UPROPERTY()
	TSet<UMInventoryComponent*> InventoriesToRepresent;
	/** Inventories belong to different actors. We store those actors' names.
	 * It is needed for replication. */
	//UPROPERTY()
	//TArray<FName> InventoriesOwnerNames;

	UPROPERTY(EditDefaultsOnly, Category=MInventoryWidgetSettings)
	TSubclassOf<UUserWidget> ItemSlotWidgetBPClass;

	UPROPERTY(EditDefaultsOnly, Category=UMDropManager)
	TSubclassOf<AMPickableActor> AMPickableItemBPClass;

	UPROPERTY(EditDefaultsOnly, Category=UMDropManager)
	TSubclassOf<UUserWidget> PickUpBarWidgetBPClass;
	
	UPROPERTY(EditDefaultsOnly, Category=UMDropManager)
	TSubclassOf<UUserWidget> InventoryWidgetBPClass;
};

