// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCGSwitch.h"

#include "PCGContext.h"
#include "PCGModule.h"
#include "PCGPin.h"
#include "PCGData.h"
#include "Elements/PCGGather.h"

#define LOCTEXT_NAMESPACE "FPCGSwitchElement"

namespace PCGSwitchConstants
{
	const FText NodeTitleBase = LOCTEXT("NodeTitleBase", "Switch");
}

void UPCGSwitchSettings::PostLoad()
{
	Super::PostLoad();
	CachePinLabels();
}

#if WITH_EDITOR
void UPCGSwitchSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Only need to change the pin labels if the options have changed
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, IntOptions) ||
		PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, StringOptions) ||
		PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, EnumSelection))
	{
		CachePinLabels();
	}
}

FText UPCGSwitchSettings::GetDefaultNodeTitle() const
{
	return PCGSwitchConstants::NodeTitleBase;
}

FText UPCGSwitchSettings::GetNodeTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Control flow node that passes through input data to a specific output pin that matches a given selection mode and corresponding 'selection' property - which can also be overridden.");
}
#endif // WITH_EDITOR

/*bool UPCGSwitchSettings::IsPinStaticallyActive(const FName& PinLabel) const
{
	// Dynamic branches are never known in advance - assume all branches are active prior to execution.
	if (IsSwitchDynamic())
	{
		return true;
	}

	FName ActiveOutputPinLabel;
	if (!GetSelectedPinLabel(ActiveOutputPinLabel))
	{
		return false;
	}

	return PinLabel == ActiveOutputPinLabel;
}

FString UPCGSwitchSettings::GetAdditionalTitleInformation() const
{
	switch (SelectionMode)
	{
		case EPCGControlFlowSelectionMode::Integer:
		{
			FString Subtitle = PCGControlFlowConstants::SubtitleInt.ToString();
			if (!IsPropertyOverriddenByPin(GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, IntegerSelection)))
			{
				Subtitle += FString::Format(TEXT(": {0}"), {IntegerSelection});
			}

			return Subtitle;
		}

		case EPCGControlFlowSelectionMode::Enum:
		{
			if (EnumSelection.Class)
			{
				FString Subtitle = EnumSelection.Class->GetName();
				if (!IsPropertyOverriddenByPin({GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, EnumSelection), GET_MEMBER_NAME_CHECKED(FEnumSelector, Value)}))
				{
					Subtitle += FString::Format(TEXT(": {0}"), {EnumSelection.Class->GetNameStringByValue(EnumSelection.Value)});
				}

				return Subtitle;
			}
			else
			{
				return PCGControlFlowConstants::SubtitleEnum.ToString();
			}
		}

		case EPCGControlFlowSelectionMode::String:
		{
			FString Subtitle = PCGControlFlowConstants::SubtitleString.ToString();
			if (!IsPropertyOverriddenByPin(GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, StringSelection)))
			{
				Subtitle += FString::Format(TEXT(": {0}"), {StringSelection});
			}

			return Subtitle;
		}

		default:
			checkNoEntry();
			break;
	}

	return PCGSwitchConstants::NodeTitleBase.ToString();
}

#if WITH_EDITOR
EPCGChangeType UPCGSwitchSettings::GetChangeTypeForProperty(const FName& InPropertyName) const
{
	EPCGChangeType ChangeType = Super::GetChangeTypeForProperty(InPropertyName) | EPCGChangeType::Cosmetic;

	if (InPropertyName == GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, bEnabled) ||
		InPropertyName == GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, IntegerSelection) ||
		InPropertyName == GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, StringSelection) ||
		InPropertyName == GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, EnumSelection) ||
		InPropertyName == GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, SelectionMode))
	{
		ChangeType |= EPCGChangeType::Structural;
	}

	return ChangeType;
}
#endif // WITH_EDITOR*/

TArray<FPCGPinProperties> UPCGSwitchSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultInputLabel,
		EPCGDataType::Any,
		/*bInAllowMultipleConnections=*/true,
		/*bAllowMultipleData=*/true,
		LOCTEXT("OutputPinTooltip", "All input will be forwarded directly to the selected output pin."));

	return PinProperties;
}

TArray<FPCGPinProperties> UPCGSwitchSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;

	switch (SelectionMode)
	{
		case EPCGControlFlowSelectionMode::Integer:
			for (const int32 Value : IntOptions)
			{
				PinProperties.Emplace(FName(FString::FromInt(Value)));
			}
			break;

		case EPCGControlFlowSelectionMode::String:
			for (const FString& Value : StringOptions)
			{
				PinProperties.Emplace(FName(Value));
			}
			break;

		case EPCGControlFlowSelectionMode::Enum:
			// -1 to bypass the MAX value
			for (int32 Index = 0; EnumSelection.Class && Index < EnumSelection.Class->NumEnums() - 1; ++Index)
			{
#if WITH_EDITOR
				bool const bHidden = EnumSelection.Class->HasMetaData(TEXT("Hidden"), Index) || EnumSelection.Class->HasMetaData(TEXT("Spacer"), Index);
				if (!bHidden)
				{
					PinProperties.Emplace(FName(EnumSelection.Class->GetDisplayNameTextByIndex(Index).ToString()));
				}
#else // WITH_EDITOR
				// HasMetaData is editor only, so there will be extra pins at runtime, but that should be okay
				PinProperties.Emplace(FName(EnumSelection.Class->GetDisplayNameTextByIndex(Index).ToString()));
#endif // WITH_EDITOR
			}
			break;

		default:
			break;
	}

	PinProperties.Emplace(PCGControlFlowConstants::DefaultPathPinLabel);

	return PinProperties;
}

FPCGElementPtr UPCGSwitchSettings::CreateElement() const
{
	return MakeShared<FPCGSwitchElement>();
}

bool UPCGSwitchSettings::IsSwitchDynamic() const
{
	bool bIsDynamic = false;

	//bIsDynamic |= (SelectionMode == EPCGControlFlowSelectionMode::Integer && IsPropertyOverriddenByPin(GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, IntegerSelection)));
	//bIsDynamic |= (SelectionMode == EPCGControlFlowSelectionMode::String && IsPropertyOverriddenByPin(GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, StringSelection)));
	//bIsDynamic |= (SelectionMode == EPCGControlFlowSelectionMode::Enum && IsPropertyOverriddenByPin({GET_MEMBER_NAME_CHECKED(UPCGSwitchSettings, EnumSelection), GET_MEMBER_NAME_CHECKED(FEnumSelector, Value)}));

	return bIsDynamic;
}

bool UPCGSwitchSettings::IsValuePresent(const int32 Value) const
{
	return IntOptions.Contains(Value);
}

bool UPCGSwitchSettings::IsValuePresent(const FString& Value) const
{
	return StringOptions.Contains(Value);
}

bool UPCGSwitchSettings::IsValuePresent(const int64 Value) const
{
	if (!EnumSelection.Class)
	{
		return false;
	}

	const int64 Index = EnumSelection.Class->GetIndexByValue(Value);
	return Index != INDEX_NONE && Index < EnumSelection.Class->NumEnums() - 1;
}

bool UPCGSwitchSettings::GetSelectedPinLabel(FName& OutSelectedPinLabel) const
{
	if (CachedPinLabels.IsEmpty())
	{
		return false;
	}

	int32 Index = INDEX_NONE;
	if (SelectionMode == EPCGControlFlowSelectionMode::Integer && IsValuePresent(IntegerSelection))
	{
		Index = IntOptions.IndexOfByKey(IntegerSelection);
	}
	else if (SelectionMode == EPCGControlFlowSelectionMode::String && IsValuePresent(StringSelection))
	{
		Index = StringOptions.IndexOfByKey(StringSelection);
	}
	else if (SelectionMode == EPCGControlFlowSelectionMode::Enum && IsValuePresent(EnumSelection.Value))
	{
		// A "hidden" value could be selected that wasn't cached, so do a name-wise comparison
		const FName PinLabel(EnumSelection.Class->GetDisplayNameTextByValue(EnumSelection.Value).ToString());
		for (int i = 0; i < CachedPinLabels.Num(); ++i)
		{
			if (CachedPinLabels[i] == PinLabel)
			{
				Index = i;
				break;
			}
		}
	}
	else
	{
		OutSelectedPinLabel = PCGControlFlowConstants::DefaultPathPinLabel;

		return true;
	}

	if (Index < 0 || Index >= CachedPinLabels.Num())
	{
		return false;
	}

	OutSelectedPinLabel = CachedPinLabels[Index];

	return true;
}

void UPCGSwitchSettings::CachePinLabels()
{
	CachedPinLabels.Empty();
	Algo::Transform(OutputPinProperties(), CachedPinLabels, [](const FPCGPinProperties& Property)
	{
		return Property.Label;
	});
}

bool FPCGSwitchElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGSwitchElement::ExecuteInternal);

	const UPCGSwitchSettings* Settings = Context->GetInputSettings<UPCGSwitchSettings>();
	check(Settings);

	FName SelectedPinLabel;
	if (!Settings->GetSelectedPinLabel(SelectedPinLabel))
	{
		PCGE_LOG_C(Error, GraphAndLog, Context, LOCTEXT("ValueDoesNotExist", "Selected value is not a valid option."));
		return true;
	}

	// Reuse the functionality of the Gather node
	//Context->OutputData = PCGGather::GatherDataForPin(Context->InputData, PCGPinConstants::DefaultInputLabel, SelectedPinLabel);

	return true;
}

#undef LOCTEXT_NAMESPACE