// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class VOXELGRAPHCORE_API FVoxelGraphMigration : public FVoxelSingleton
{
public:
	UScriptStruct* FindNewNode(FName CachedName) const;
	UFunction* FindNewFunction(FName CachedName) const;
	FName FindNewPinName(UObject* Outer, FName OldName) const;
	void OnNodeMigrated(FName CachedName, UEdGraphNode* GraphNode) const;

public:
	struct FNodeMigration
	{
		FName DisplayName;
		UFunction* NewFunction = nullptr;
		UScriptStruct* NewNode = nullptr;
	};
	struct FPinMigration
	{
		UObject* Outer = nullptr;
		FName OldName;
		FName NewName;
	};
	void RegisterNodeMigration(const FNodeMigration& NodeMigration);
	void RegisterPinMigration(const FPinMigration& PinMigration);
	void RegisterOnNodeMigrated(FName CachedName, TFunction<void(UEdGraphNode*)> Lambda);

private:
	TMap<FName, FNodeMigration> NameToNodeMigration;
	TMap<TPair<UObject*, FName>, FPinMigration> NameToPinMigration;
	TMap<FName, TFunction<void(UEdGraphNode*)>> NameToOnNodeMigrated;
};

extern VOXELGRAPHCORE_API FVoxelGraphMigration* GVoxelGraphMigration;

#define REGISTER_VOXEL_NODE_MIGRATION(Name, Node) \
	GVoxelGraphMigration->RegisterNodeMigration({ Name, nullptr, StaticStructFast<Node>() });

#define REGISTER_VOXEL_NODE_PIN_MIGRATION(Node, OldPin, NewPin) \
	INTELLISENSE_ONLY({ int32 OldPin; (void)OldPin; Node().NewPin; }); \
	{ \
		FString OldPinName = #OldPin; \
		FString NewPinName = #NewPin; \
		ensure(OldPinName.RemoveFromEnd("Pin")); \
		ensure(NewPinName.RemoveFromEnd("Pin")); \
		GVoxelGraphMigration->RegisterPinMigration({ StaticStructFast<Node>(), *OldPinName, *NewPinName }); \
	}


#define REGISTER_VOXEL_FUNCTION_MIGRATION(Name, Class, Function) \
	GVoxelGraphMigration->RegisterNodeMigration({ Name, FindUFunctionChecked(Class, Function), nullptr });

#define REGISTER_VOXEL_FUNCTION_PIN_MIGRATION(Class, Function, OldPin, NewPin) \
	INTELLISENSE_ONLY({ int32 OldPin, NewPin; (void)OldPin; (void)NewPin; }); \
	GVoxelGraphMigration->RegisterPinMigration({ FindUFunctionChecked(Class, Function), #OldPin, #NewPin });