// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphMigration.h"

FVoxelGraphMigration* GVoxelGraphMigration = new FVoxelGraphMigration();

UScriptStruct* FVoxelGraphMigration::FindNewNode(const FName CachedName) const
{
	const FNodeMigration* Migration = NameToNodeMigration.Find(CachedName);
	if (!Migration ||
		!Migration->NewNode)
	{
		if (!Migration)
		{
			LOG_VOXEL(Warning, "No redirector for '%s'", *CachedName.ToString());
		}
		return nullptr;
	}

	LOG_VOXEL(Log, "Redirecting '%s' to %s", *CachedName.ToString(), *Migration->NewNode->GetName());
	return Migration->NewNode;
}

UFunction* FVoxelGraphMigration::FindNewFunction(const FName CachedName) const
{
	const FNodeMigration* Migration = NameToNodeMigration.Find(CachedName);
	if (!Migration ||
		!Migration->NewFunction)
	{
		if (!Migration)
		{
			LOG_VOXEL(Warning, "No redirector for '%s'", *CachedName.ToString());
		}
		return nullptr;
	}

	LOG_VOXEL(Log, "Redirecting '%s' to %s", *CachedName.ToString(), *Migration->NewFunction->GetName());
	return Migration->NewFunction;
}

FName FVoxelGraphMigration::FindNewPinName(UObject* Outer, const FName OldName) const
{
	if (OldName.ToString().StartsWith("ORPHANED_"))
	{
		return {};
	}

	const FPinMigration* Migration = NameToPinMigration.Find({ Outer, OldName });
	if (!Migration)
	{
		LOG_VOXEL(Warning, "No redirector for %s.%s", *Outer->GetName(), *OldName.ToString());
		return {};
	}

	LOG_VOXEL(Log, "Redirecting %s.%s to %s",
		*Outer->GetName(),
		*OldName.ToString(),
		*Migration->NewName.ToString());
	return Migration->NewName;
}

void FVoxelGraphMigration::OnNodeMigrated(const FName CachedName, UEdGraphNode* GraphNode) const
{
	if (const TFunction<void(UEdGraphNode*)> Lambda = NameToOnNodeMigrated.FindRef(CachedName))
	{
		Lambda(GraphNode);
	}
}

void FVoxelGraphMigration::RegisterNodeMigration(const FNodeMigration& NodeMigration)
{
	ensure(!NameToNodeMigration.Contains(NodeMigration.DisplayName));
	NameToNodeMigration.Add(NodeMigration.DisplayName, NodeMigration);
}

void FVoxelGraphMigration::RegisterPinMigration(const FPinMigration& PinMigration)
{
	ensure(!PinMigration.OldName.ToString().EndsWith("Pin"));
	ensure(!PinMigration.NewName.ToString().EndsWith("Pin"));

	if (const UFunction* Function = Cast<UFunction>(PinMigration.Outer))
	{
		ensure(Function->FindPropertyByName(PinMigration.NewName));
	}

	const TPair<UObject*, FName> Key{ PinMigration.Outer, PinMigration.OldName };
	ensure(!NameToPinMigration.Contains(Key));
	NameToPinMigration.Add(Key, PinMigration);
}

void FVoxelGraphMigration::RegisterOnNodeMigrated(const FName CachedName, const TFunction<void(UEdGraphNode*)> Lambda)
{
	ensure(!NameToOnNodeMigrated.Contains(CachedName));
	NameToOnNodeMigrated.Add(CachedName, Lambda);
}