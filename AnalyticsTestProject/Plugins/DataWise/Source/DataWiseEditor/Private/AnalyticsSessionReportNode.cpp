#include "AnalyticsSessionReportNode.h"
#include "DataWiseEditor.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "AnalyticsSession.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintCompilationManager.h"
#include "Kismet/GameplayStatics.h"
#include "BlueprintActionFilter.h"

#define LOCTEXT_NAMESPACE "AnalyticsEditorModule"

UK2_AnalyticsSessionReportNode::UK2_AnalyticsSessionReportNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UK2_AnalyticsSessionReportNode::AllocateDefaultPins() 
{
	UEdGraphPin* EnterPin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	UEdGraphPin* ExitPin = CreatePin(EEdGraphPinDirection::EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	UEdGraphPin* TargetPin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Object, UAnalyticsSession::StaticClass(), FName("Session"));
	UEdGraphPin* ClassPin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Class, UAnalyticsPacket::StaticClass(), FName("Class"));
	ClassPin->bNotConnectable = true;

	Super::AllocateDefaultPins();
}

FText UK2_AnalyticsSessionReportNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString("Report Event");
}

void UK2_AnalyticsSessionReportNode::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	if (ActionRegistrar.IsOpenForRegistration(GetClass()))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		ActionRegistrar.AddBlueprintAction(GetClass(), NodeSpawner);
	}
}

void UK2_AnalyticsSessionReportNode::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	if(Pin == FindPin(FName("Class")))
	{
		TArray<UEdGraphPin*> OldPins = Pins;

		CreateClassPins(FindPacketClass(Pins));

		RestoreSplitPins(OldPins);
	}
}

void UK2_AnalyticsSessionReportNode::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	AllocateDefaultPins();

	CreateClassPins(FindPacketClass(OldPins));

	RestoreSplitPins(OldPins);
}

void UK2_AnalyticsSessionReportNode::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();
	
	CreateClassPins(FindPacketClass(Pins));
}

void UK2_AnalyticsSessionReportNode::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	if (Pin == FindPin(FName("Class")))
	{
		PinDefaultValueChanged(Pin);
	}
}

TArray<UEdGraphPin*> UK2_AnalyticsSessionReportNode::FindClassPins()
{
	TArray<UEdGraphPin*> found;
	for (UEdGraphPin* pin : Pins)
	{
		if (pin != GetExecPin() && pin != FindPin(UEdGraphSchema_K2::PN_Then) && pin != FindPin(FName("Session")) && pin != FindPin(FName("Class")))
		{
			found.Add(pin);
		}
	}
	return found;
}

void UK2_AnalyticsSessionReportNode::CreateClassPins(UClass* selected_class)
{
	TArray<UEdGraphPin*> class_pins = FindClassPins();
	for (UEdGraphPin* class_pin : class_pins)
	{
		class_pin->BreakAllPinLinks(true);
		Pins.Remove(class_pin);
	}

	TArray<UEdGraphPin*> OldPins = Pins;

	if (selected_class != nullptr)
	{
		TArray<FString> base_data;
		
		for (TFieldIterator<UProperty> property_iterator(UAnalyticsPacket::StaticClass()); property_iterator; ++property_iterator)
		{
			UProperty* prop = *property_iterator;
			FString name = prop->GetNameCPP();
			base_data.Add(name);
		}
		

		selected_class->FlushCompilationQueueForLevel();

		for (TFieldIterator<UProperty> property_iterator(selected_class); property_iterator; ++property_iterator)
		{
			UProperty* prop = *property_iterator;
			FString type = prop->GetCPPType();
			FString name = prop->GetNameCPP();
			bool is_public = !prop->HasAllPropertyFlags(CPF_DisableEditOnInstance);

			if (base_data.Contains(name)) continue;

			//TODO Fix property flags not being loaded in time causing CPF_DisableEditOnInstance to be absent
			//if (!is_public) continue;

			UEdGraphPin* new_class_pin = nullptr;

			if (type.Equals("FString")) {
				new_class_pin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_String, FName(*name));
			}
			else if (type.Equals("FVector")) {
				new_class_pin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get(), FName(*name));
			}
			else if (type.Equals("int32")) {
				new_class_pin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Int, FName(*name));
			}
			else if (type.Equals("bool")) {
				new_class_pin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Boolean, FName(*name));
			}
			else if (type.Equals("float")) {
				new_class_pin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Float, FName(*name));
			}
			else if (type.Equals("FVector2D")) {
				new_class_pin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector2D>::Get(), FName(*name));
			}
			else
			{
				UE_LOG(AnalyticsLogEditor, Error, TEXT("Unsupported property type: %s (%s)"), *type, *name);
			}

			if (new_class_pin != nullptr)
			{
				FText tooltip = prop->GetToolTipText();
				if(!tooltip.IsEmpty()) new_class_pin->PinToolTip = tooltip.ToString();
			}
		}

		RestoreSplitPins(OldPins);
		GetGraph()->NotifyGraphChanged();
		FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
	}
}

int UK2_AnalyticsSessionReportNode::CheckPins(FKismetCompilerContext* context, UClass* selected_class)
{
	int pins = 0;

	if (selected_class != nullptr)
	{
		TArray<FString> base_data;

		for (TFieldIterator<UProperty> property_iterator(UAnalyticsPacket::StaticClass()); property_iterator; ++property_iterator)
		{
			UProperty* prop = *property_iterator;
			FString name = prop->GetNameCPP();
			base_data.Add(name);
		}


		for (TFieldIterator<UProperty> property_iterator(selected_class); property_iterator; ++property_iterator)
		{
			UProperty* prop = *property_iterator;
			FString type = prop->GetCPPType();
			FString name = prop->GetNameCPP();
			bool is_public = !prop->HasAllPropertyFlags(CPF_DisableEditOnInstance);

			if (base_data.Contains(name)) continue;
			if (!is_public) continue;

			UEdGraphPin* new_class_pin = nullptr;

			if (type.Equals("FString")) {
				pins++;
			}
			else if (type.Equals("FVector")) {
				pins++;
			}
			else if (type.Equals("int32")) {
				pins++;
			}
			else if (type.Equals("bool")) {
				pins++;
			}
			else if (type.Equals("float")) {
				pins++;
			}
			else if (type.Equals("FVector2D")) {
				pins++;
			}
			else
			{
				if(context != nullptr)
				{
					context->MessageLog.Warning(*FText::Format(FText::FromString("Report node @@ cannot process unsupported parameter '{0}' of type '{1}' in @@"), FText::FromString(name), FText::FromString(type)).ToString(), this, selected_class);
				}
			}
		}
	}

	return pins;
}

UClass* UK2_AnalyticsSessionReportNode::FindPacketClass(TArray<UEdGraphPin*>& search_pins)
{
	for(UEdGraphPin* class_pin : search_pins)
	{
		if(class_pin->GetName().Equals("Class"))
		{
			if (class_pin->DefaultObject == nullptr) continue;

			return CastChecked<UClass>(class_pin->DefaultObject);
			
		}
	}

	return nullptr;
}

FText UK2_AnalyticsSessionReportNode::GetMenuCategory() const
{
	return FText::FromString("Analytics Session");
}



FBlueprintNodeSignature UK2_AnalyticsSessionReportNode::GetSignature() const
{
	FBlueprintNodeSignature NodeSignature = Super::GetSignature();

	return NodeSignature;
}

bool UK2_AnalyticsSessionReportNode::IsActionFilteredOut(const FBlueprintActionFilter& filter)
{
	for(FBlueprintActionFilter::FTargetClassFilterData fd : filter.TargetClasses)
	{
		if (fd.TargetClass == UAnalyticsSession::StaticClass()) return false;
	}

	return true;
}

bool UK2_AnalyticsSessionReportNode::ShowPaletteIconOnNode() const
{
	return false;
}


void UK2_AnalyticsSessionReportNode::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) 
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UEdGraphPin* pin_exec = GetExecPin();
	UEdGraphPin* pin_then = FindPin(UEdGraphSchema_K2::PN_Then);
	UEdGraphPin* pin_target = FindPin(FName("Session"));
	UEdGraphPin* pin_class = FindPin(FName("Class"));

	CheckPins(&CompilerContext, FindPacketClass(Pins));

	// Create construction node

	UK2Node_CallFunction* ConstructNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	ConstructNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, SpawnObject), UGameplayStatics::StaticClass());
	ConstructNode->AllocateDefaultPins();

	UEdGraphPin* pin_exec_construct = ConstructNode->GetExecPin();
	UEdGraphPin* pin_then_construct = ConstructNode->GetThenPin();
	UEdGraphPin* pin_class_construct = ConstructNode->FindPin(TEXT("ObjectClass"));
	UEdGraphPin* pin_target_construct = ConstructNode->FindPin(TEXT("Outer"));
	UEdGraphPin* pin_result_construct = ConstructNode->GetReturnValuePin();

	CompilerContext.MovePinLinksToIntermediate(*pin_exec, *pin_exec_construct);
	CompilerContext.MovePinLinksToIntermediate(*pin_class, *pin_class_construct);
	CompilerContext.CopyPinLinksToIntermediate(*pin_target, *pin_target_construct);

	// Create assignment nodes

	UEdGraphPin* pin_then_assignment = pin_then_construct;
	TArray<UEdGraphPin*> class_pins = FindClassPins();
	for (UEdGraphPin* class_pin : class_pins)
	{
		UK2Node_CallFunction* assignment_node = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		assignment_node->SetFromFunction(UEdGraphSchema_K2::FindSetVariableByNameFunction(class_pin->PinType));
		assignment_node->AllocateDefaultPins();

		pin_then_assignment->MakeLinkTo(assignment_node->GetExecPin());
		pin_then_assignment = assignment_node->GetThenPin();

		UEdGraphPin* PropertyNamePin = assignment_node->FindPinChecked(TEXT("PropertyName"));
		PropertyNamePin->DefaultValue = class_pin->PinName.ToString();

		UEdGraphPin* ObjectPin = assignment_node->FindPin(TEXT("Object"));
		pin_result_construct->MakeLinkTo(ObjectPin);

		UEdGraphPin* pin_value_assignment = assignment_node->FindPinChecked(TEXT("Value"));
		if(class_pin->LinkedTo.Num()==0)
		{
			pin_value_assignment->DefaultValue = class_pin->DefaultValue;
		} 
		else
		{
			pin_value_assignment->PinType.PinCategory = class_pin->PinType.PinCategory;
			pin_value_assignment->PinType.PinSubCategory = class_pin->PinType.PinSubCategory;
			pin_value_assignment->PinType.PinSubCategoryObject = class_pin->PinType.PinSubCategoryObject;
			CompilerContext.MovePinLinksToIntermediate(*class_pin, *pin_value_assignment);
		}
	}

	// Create report node

	UK2Node_CallFunction* ReportNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	ReportNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UAnalyticsSession, ReportEvent), UAnalyticsSession::StaticClass());
	ReportNode->AllocateDefaultPins();

	UEdGraphPin* pin_exec_report = ReportNode->GetExecPin();
	UEdGraphPin* pin_then_report = ReportNode->GetThenPin();
	UEdGraphPin* pin_target_report = ReportNode->FindPin(UEdGraphSchema_K2::PN_Self);
	UEdGraphPin* pin_packet_report = ReportNode->FindPin(TEXT("packet"));

	pin_then_assignment->MakeLinkTo(pin_exec_report);
	pin_result_construct->MakeLinkTo(pin_packet_report);
	CompilerContext.MovePinLinksToIntermediate(*pin_target, *pin_target_report);
	CompilerContext.MovePinLinksToIntermediate(*pin_then, *pin_then_report);

	BreakAllNodeLinks();
}

#undef LOCTEXT_NAMESPACE