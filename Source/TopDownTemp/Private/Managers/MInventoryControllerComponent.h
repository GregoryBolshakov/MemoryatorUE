#pragma once

#include "CoreMinimal.h"
#include "Components/MInventoryComponent.h"
#include "MInventoryControllerComponent.generated.h"

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

/** Component for MPlayerController for managing loot, dragging/dropping items. Handles PickUpBar and Inventory widgets */
UCLASS(Blueprintable)
class UMInventoryControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TryDropDraggedOnTheGround();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TryStoreDraggedToAnySlot(FMUid InventoryOwnerActorUid);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TryStoreDraggedToSpecificSlot(FMUid InventoryOwnerActorUid, int SlotNumberInArray);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TryDragItemFromSpecificSlot(FMUid InventoryOwnerActorUid, int SlotNumberInArray, int Quantity);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TrySwapDraggedWithSpecificSlot(FMUid InventoryOwnerActorUid, int SlotNumberInArray);

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
	inline UMInventoryComponent* GetMyInventory() const;

	UPROPERTY()
	FItem DraggedItem;

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

