// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class DataWise : ModuleRules
	{
        public DataWise(ReadOnlyTargetRules Target) : base(Target)
		{
		    PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
                    "RenderCore"
                }
			);
        }
	}
}
