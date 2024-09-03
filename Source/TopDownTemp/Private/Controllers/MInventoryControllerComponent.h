#pragma once

#include "CoreMinimal.h"
#include "Components/MInventoryComponent.h"
#include "UI/MCommunicationWidget.h"
#include "MInventoryControllerComponent.generated.h"

//TODO: Move to proper folder

class UMCommunicationWidget;
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
	void Server_TryDropDraggedOnTheGround(const EInventoryType InventoryType);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TryStoreDraggedToAnySlot(FMUid InventoryOwnerActorUid, const EInventoryType InventoryType);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TryStoreDraggedToSpecificSlot(FMUid InventoryOwnerActorUid, int SlotNumberInArray, const EInventoryType InventoryType);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TryDragItemFromSpecificSlot(FMUid InventoryOwnerActorUid, int SlotNumberInArray, int Quantity, const EInventoryType InventoryType);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TrySwapDraggedWithSpecificSlot(FMUid InventoryOwnerActorUid, int SlotNumberInArray, const EInventoryType InventoryType);

	UFUNCTION(Client, Reliable)
	void Client_AddInventoryForPickUp(const UMInventoryComponent* ReplicatedInventory);

	UFUNCTION(Client, Reliable)
	void Client_RemoveInventoryForPickUp(const FMUid InventoryOwnerUid);

	UFUNCTION(Client, Reliable)
	void Client_UpdatePickUpBar() const;

	void UpdateInventoryWidget() const;

	UFUNCTION(Client, Reliable)
	void Client_UpdateCommunicationWidget() const;

	bool ContainsPickUpInventory(const UMInventoryComponent* ReplicatedInventory) const { return InventoriesToRepresent.Contains(ReplicatedInventory); }

	UFUNCTION(BlueprintCallable)
	void CreateOrShowInventoryWidget();

	UFUNCTION(Client, Reliable)
	void Client_CreateCommunicationWidget();

	UFUNCTION(Client, Reliable)
	void Client_CloseCommunicationWidget();

	// TODO: Make it clearer. They are not really replicated. It seems like they get set on both sides independently
	/** A set of replicated inventories player is in contact with. */
	UPROPERTY()
	TSet<const UMInventoryComponent*> InventoriesToRepresent;

private:
	inline UMInventoryComponent* GetMyInventory(EInventoryType InventoryType) const;

	/** Server only */
	UPROPERTY()
	FItem DraggedItem;

	/** Inventory widget. Never destroy it but only hide/show. */
	UPROPERTY()
	UMInventoryWidget* InventoryWidget;

	/** Communication widget. Always create/destroy it instead of show/hide. */
	UPROPERTY()
	UMCommunicationWidget* CommunicationWidget;

	/** Screen-side widget with drop available to pick up.\n
	 * We never destroy it but only hide/show. Two main reasons for that:\n
	 * 1. Drag-n-drop might happen after widget is destroyed and won't be resolved;
	 * 2. Avoid overhead of creating and destroying widgets */
	UPROPERTY()
	UMPickUpBarWidget* PickUpBarWidget;
};

