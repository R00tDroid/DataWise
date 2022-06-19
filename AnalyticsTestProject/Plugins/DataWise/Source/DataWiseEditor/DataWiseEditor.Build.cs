// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class DataWiseEditor : ModuleRules
	{
        public DataWiseEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
                    "RenderCore",
				    "UnrealEd",
				    "KismetWidgets",
				    "KismetCompiler",
				    "BlueprintGraph",
				    "GraphEditor",
				    "Kismet",
                    "Slate",
                    "UMG",
                    "InputCore",
					"RHI",
                    "AdvancedPreviewScene",
                    "ShaderCore",
                    "DataWise"
                }
			);

            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "Slate", "SlateCore", "EditorStyle", "DataWise" });
        }
	}
}
