// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelGraph.h"
#include "VoxelAssetTypeActions.h"

class FVoxelGraphAssetTypeActions : public FVoxelInstanceAssetTypeActions
{
public:
	FVoxelGraphAssetTypeActions() = default;

	//~ Begin FVoxelInstanceAssetTypeActions Interface
	virtual UClass* GetInstanceClass() const override
	{
		return UVoxelGraph::StaticClass();
	}
	virtual FSlateIcon GetInstanceActionIcon() const override
	{
		return FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.BrushComponent");
	}
	virtual void SetParent(UObject* InstanceAsset, UObject* ParentAsset) const override
	{
		CastChecked<UVoxelGraph>(InstanceAsset)->SetBaseGraph(CastChecked<UVoxelGraph>(ParentAsset));
	}
	//~ End FVoxelInstanceAssetTypeActions Interface
};

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterGraphAssetTypeActions)
{
	FVoxelAssetTypeActions::Register(
		UVoxelGraph::StaticClass(),
		MakeVoxelShared<FVoxelGraphAssetTypeActions>());
}