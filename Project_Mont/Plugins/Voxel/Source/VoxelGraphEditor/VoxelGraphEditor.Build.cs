// Copyright Voxel Plugin SAS. All Rights Reserved.

using UnrealBuildTool;

public class VoxelGraphEditor : ModuleRules_Voxel
{
    public VoxelGraphEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "VoxelGraphCore",
                "VoxelContentEditor",
                "UMG",
                "HTTP",
                "MainFrame",
                "ToolMenus",
                "MessageLog",
                "CurveEditor",
                "GraphEditor",
                "KismetWidgets",
                "EditorWidgets",
                "BlueprintGraph",
                "ApplicationCore",
                "SlateRHIRenderer",
                "SharedSettingsWidgets",
                "WorkspaceMenuStructure",
                "InteractiveToolsFramework",
                "EditorInteractiveToolsFramework",
            }
        );
    }
}