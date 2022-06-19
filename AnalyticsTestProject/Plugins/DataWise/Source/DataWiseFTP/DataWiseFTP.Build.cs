// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class DataWiseFTP : ModuleRules
	{
        public DataWiseFTP(ReadOnlyTargetRules Target) : base(Target)
		{
		    PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
                    "RenderCore",
                    "DataWise"
                }
			);
        }
    }
}
