// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PCGControlFlow.h"
#include "PCGSettings.h"

#include "PCGSwitch.generated.h"

/**
 * Routes data from the input pin, to a specific output pin based on a selection criteria (Int/String/Enum) 
 */
UCLASS(BlueprintType, ClassGroup = (Procedural), meta=(Keywords = "if multi enum branch"))
class UPCGSwitchSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	//~Begin UObject interface
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~End UObject interface

	//~Begin UPCGSettings interface
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("Switch")); }
	virtual FText GetDefaultNodeTitle() const override;
	virtual FText GetNodeTooltipText() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::ControlFlow; }
	virtual bool HasDynamicPins() const override { return true; }
#endif // WITH_EDITOR

	//virtual bool IsPinStaticallyActive(const FName& PinLabel) const override;
	//virtual FString GetAdditionalTitleInformation() const override;
	//virtual bool HasFlippedTitleLines() const override { return true; }

protected:
#if WITH_EDITOR
	//virtual EPCGChangeType GetChangeTypeForProperty(const FName& InPropertyName) const override;
#endif
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

	/** Switch is dynamic if selection value is overridden / cannot be determined prior to execution. */
	virtual bool IsSwitchDynamic() const;

public:
	/** Determines the type of value to be used to select an output. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings)
	EPCGControlFlowSelectionMode SelectionMode = EPCGControlFlowSelectionMode::Integer;

	/** Determines which output will be selected if the selection mode is Integer. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings, meta=(PCG_Overridable, EditConditionHides, EditCondition="SelectionMode == EPCGControlFlowSelectionMode::Integer"))
	int32 IntegerSelection = 0;

	/** Determines the available output pin selection options. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings, meta=(EditConditionHides, EditCondition="SelectionMode == EPCGControlFlowSelectionMode::Integer"))
	TArray<int32> IntOptions = {0};

	/** Determines which output will be selected if the selection mode is String. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings, meta=(PCG_Overridable, EditConditionHides, NoResetToDefault, EditCondition="SelectionMode == EPCGControlFlowSelectionMode::String"))
	FString StringSelection;

	/** Determines the available output pin selection options. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings, meta=(EditConditionHides, EditCondition="SelectionMode == EPCGControlFlowSelectionMode::String"))
	TArray<FString> StringOptions;

	/** Determines which output pin will be selected if the selection mode is Enum. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Settings, meta=(PCG_Overridable, EditConditionHides, EditCondition="SelectionMode == EPCGControlFlowSelectionMode::Enum"))
	FEnumSelector EnumSelection;

	/** Cached pin labels for use during the selection process. */
	TArray<FName> CachedPinLabels;

	/** Returns true if the integer value exists in the user defined options. */
	bool IsValuePresent(int32 Value) const;

	/** Returns true if the string value exists in the user defined options. */
	bool IsValuePresent(const FString& Value) const;

	/** Returns true if the enum value exists within the selected enum class. */
	bool IsValuePresent(int64 Value) const;

	/** Helper function to use the appropriate selection value to determine the current selection. Returns true if it succeeds or false if the value is invalid. */
	bool GetSelectedPinLabel(FName& OutSelectedPinLabel) const;

	void CachePinLabels();
};

class FPCGSwitchElement : public IPCGElement
{
public:
	int TempElemNotToBeAbstract = 0;
	void TempFuncNotToBeAbstract() { TempElemNotToBeAbstract++; }
	virtual FPCGContext* Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node) override { return {}; }

protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
