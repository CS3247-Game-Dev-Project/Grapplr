// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class CS3247_Group2 : ModuleRules
{
	public CS3247_Group2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			
			// Mass Entity related
			"MassEntity", 
			"MassCommon", 
			"MassMovement", 
			"MassRepresentation",
			"MassNavigation", 
			"MassActors",
			"MassSpawner",
			"MassAIBehavior",
			"MassNavMeshNavigation",
			"MassSignals",
			
			// State tree
			"StateTreeModule",
			"GameplayStateTreeModule",
			
			// For navmesh based spawning (not used?)
			"NavigationSystem",
			
			// For baking the signed distance fields
			"GeometryScriptingCore",
			"GeometryFramework",
			"GeometryFlowCore",
			"GeometryCore",
			"GeometryFlowMeshProcessing",
			"DynamicMesh",
			"MeshDescription"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "LevelEditor" });
		
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
