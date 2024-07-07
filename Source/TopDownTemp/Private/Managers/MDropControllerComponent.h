#pragma once

#include "CoreMinimal.h"
#include "Components/MInventoryComponent.h"
#include "MDropControllerComponent.generated.h"

class UMInventoryWidget;
class AMPlayerController;
class UMPickUpBarWidget;
class UMInventoryComponent;
class UUserWidget;
class AMPickableActor;
struct FBundle;

USTRUCT(BlueprintType)
struct FSlotsWrapper
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FSlot> Slots;
};

/** An extension for MPlayerController for managing loot, dragging/dropping items. */
UCLASS(Blueprintable)
class UMDropControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void AddInventoryForPickUp(const UMInventoryComponent* ReplicatedInventory);

	void RemoveInventoryForPickUp(const UMInventoryComponent* ReplicatedInventory);

	void UpdatePickUpBar() const;

	void UpdateInventoryWidget() const;

	bool ContainsPickUpInventory(const UMInventoryComponent* ReplicatedInventory) const { return InventoriesToRepresent.Contains(ReplicatedInventory); }

	UFUNCTION(BlueprintCallable)
	void CreateOrShowInventoryWidget();

	/** A set of replicated inventories player is in contact with. */
	UPROPERTY()
	TSet<const UMInventoryComponent*> InventoriesToRepresent;

private:
	/** Inventory widget. Never destroy it but only hide/show. */
	UPROPERTY()
	UMInventoryWidget* InventoryWidget;

	/** Screen-side widget with drop available to pick up.\n
	 * We never destroy it but only hide/show. Two main reasons for that:\n
	 * 1. Drag-n-drop might happen after widget is destroyed and won't be resolved;
	 * 2. Avoid overhead of creating and destroying widgets */
	UPROPERTY()
	UMPickUpBarWidget* PickUpBarWidget;
};

