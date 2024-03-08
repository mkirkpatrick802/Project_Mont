// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelParameter.h"
#include "VoxelParameterOverridesOwner.h"
#include "VoxelParameterOverrideCollection_DEPRECATED.h"
#include "VoxelGraph.generated.h"

struct FVoxelGraphMetadata;
class UVoxelTerminalGraph;
class UVoxelParameterContainer_DEPRECATED;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelEditedDocumentInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<UEdGraph> EdGraph;

	UPROPERTY()
	FVector2D ViewLocation = FVector2D::ZeroVector;

	UPROPERTY()
	float ZoomAmount = -1.f;
};

UENUM()
enum class EVoxelGraphParameterType_DEPRECATED
{
	Parameter,
	Input,
	Output,
	LocalVariable
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelGraphParameter_DEPRECATED : public FVoxelParameter
{
	GENERATED_BODY()

	UPROPERTY()
	bool bExposeInputDefaultAsPin_DEPRECATED = false;

	UPROPERTY(EditAnywhere, Category = "Voxel")
	EVoxelGraphParameterType_DEPRECATED ParameterType = {};
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

constexpr FVoxelGuid GVoxelMainTerminalGraphGuid = MAKE_VOXEL_GUID("00000000FFFFFFFF000000002A276A92");

UCLASS(BlueprintType, meta = (VoxelAssetType, AssetColor=Blue))
class VOXELGRAPHCORE_API UVoxelGraph
	: public UObject
	, public IVoxelParameterOverridesObjectOwner
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Config")
	FString Category;

	UPROPERTY(EditAnywhere, Category = "Config")
	FString Description;

	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	FString DisplayNameOverride;

	// Enable to render a custom graph thumbnail
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	bool bEnableThumbnail = false;

	// Display graph in graph creation window
	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	bool bUseAsTemplate = false;

	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	bool bShowInContextMenu = true;
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TArray<FVoxelEditedDocumentInfo> LastEditedDocuments;
#endif

	UVoxelGraph();

public:
#if WITH_EDITOR
	FVoxelGraphMetadata GetMetadata() const;
#endif

public:
	UVoxelTerminalGraph* FindTerminalGraph_NoInheritance(const FGuid& Guid);
	const UVoxelTerminalGraph* FindTerminalGraph_NoInheritance(const FGuid& Guid) const;

	void ForeachTerminalGraph_NoInheritance(TFunctionRef<void(UVoxelTerminalGraph&)> Lambda);
	void ForeachTerminalGraph_NoInheritance(TFunctionRef<void(const UVoxelTerminalGraph&)> Lambda) const;

	FGuid FindTerminalGraphGuid_NoInheritance(const UVoxelTerminalGraph* TerminalGraph) const;

public:
	UVoxelTerminalGraph* FindTerminalGraph(const FGuid& Guid);
	const UVoxelTerminalGraph* FindTerminalGraph(const FGuid& Guid) const;

	UVoxelTerminalGraph& FindTerminalGraphChecked(const FGuid& Guid);
	const UVoxelTerminalGraph& FindTerminalGraphChecked(const FGuid& Guid) const;

	TVoxelSet<FGuid> GetTerminalGraphs() const;
	// Will also return true if it's already overridden
	bool IsTerminalGraphOverrideable(const FGuid& Guid) const;
	const UVoxelTerminalGraph* FindTopmostTerminalGraph(const FGuid& Guid) const;

#if WITH_EDITOR
	UVoxelTerminalGraph& AddTerminalGraph(
		const FGuid& Guid,
		const UVoxelTerminalGraph* Template = nullptr);
	void RemoveTerminalGraph(const FGuid& Guid);
	void ReorderTerminalGraphs(TConstVoxelArrayView<FGuid> NewGuids);
#endif

public:
	const FVoxelParameter* FindParameter(const FGuid& Guid) const;
	const FVoxelParameter& FindParameterChecked(const FGuid& Guid) const;

	TVoxelSet<FGuid> GetParameters() const;
	bool IsInheritedParameter(const FGuid& Guid) const;

#if WITH_EDITOR
	void AddParameter(const FGuid& Guid, const FVoxelParameter& Parameter);
	void RemoveParameter(const FGuid& Guid);
	void UpdateParameter(const FGuid& Guid, TFunctionRef<void(FVoxelParameter&)> Update);
	void ReorderParameters(TConstVoxelArrayView<FGuid> NewGuids);
#endif

private:
	UPROPERTY()
	TMap<FGuid, TObjectPtr<UVoxelTerminalGraph>> GuidToTerminalGraph;

	UPROPERTY()
	TMap<FGuid, FVoxelParameter> GuidToParameter;

public:
	void Fixup();
	bool IsFunctionLibrary() const;
	UVoxelTerminalGraph& GetMainTerminalGraph();
	const UVoxelTerminalGraph& GetMainTerminalGraph() const;

	static void LoadAllGraphs();
	TVoxelSet<const UVoxelGraph*> GetUsedGraphs() const;

private:
	void GetUsedGraphsImpl(TVoxelSet<const UVoxelGraph*>& OutGraphs) const;

public:
	// Won't check for loops, prefer GetBaseGraphs instead
	UVoxelGraph* GetBaseGraph_Unsafe() const;

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void SetBaseGraph(UVoxelGraph* NewBaseGraph);
#endif

	// Includes self
	TVoxelInlineSet<UVoxelGraph*, 8> GetBaseGraphs();
	TVoxelInlineSet<const UVoxelGraph*, 8> GetBaseGraphs() const;

	// Includes self, slow
	TVoxelSet<UVoxelGraph*> GetChildGraphs_LoadedOnly();
	TVoxelSet<const UVoxelGraph*> GetChildGraphs_LoadedOnly() const;

private:
	UPROPERTY()
	TObjectPtr<UVoxelGraph> PrivateBaseGraph;

	UPROPERTY()
	FVoxelParameterOverrides ParameterOverrides;

public:
	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual void PostInitProperties() override;
	virtual void PostCDOContruct() override;
#if WITH_EDITOR
	virtual void PostEditUndo() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostRename(UObject* OldOuter, FName OldName) override;
#endif
	//~ End UObject Interface

	//~ Begin IVoxelParameterOverridesOwner Interface
	virtual bool ShouldForceEnableOverride(const FVoxelParameterPath& Path) const override;
	virtual UVoxelGraph* GetGraph() const override { return ConstCast(this); }
	virtual FVoxelParameterOverrides& GetParameterOverrides() override;
	//~ End IVoxelParameterOverridesOwner Interface

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	bool bEnableNodeRangeStats = false;
#endif

private:
#if WITH_EDITORONLY_DATA
	// 320, removal of UVoxelGraphAsset
	UPROPERTY()
	TObjectPtr<UVoxelGraph> Graph_DEPRECATED;

	// Old
	UPROPERTY()
	TMap<FName, TObjectPtr<UEdGraph>> Graphs_DEPRECATED;

	// 341, addition of UVoxelTerminalGraph
	UPROPERTY()
	TObjectPtr<UEdGraph> MainEdGraph_DEPRECATED;

	// 341, addition of UVoxelTerminalGraph
	UPROPERTY()
	bool bEnableNameOverride_DEPRECATED  = false;

	// 341, addition of UVoxelTerminalGraph
	UPROPERTY()
	FString NameOverride_DEPRECATED;

	// 341, addition of UVoxelTerminalGraph
	UPROPERTY()
	bool bExposeToLibrary_DEPRECATED = true;

	// 341, addition of UVoxelTerminalGraph
	UPROPERTY()
	TArray<TObjectPtr<UVoxelGraph>> InlineMacros_DEPRECATED;

	// 341, addition of UVoxelTerminalGraph
	UPROPERTY()
	TArray<FVoxelGraphParameter_DEPRECATED> Parameters_DEPRECATED;

	// 341, removal of UVoxelGraphInstance
	UPROPERTY()
	TObjectPtr<UVoxelGraph> Parent_DEPRECATED;

	// 341, removal of UVoxelGraphInstance
	UPROPERTY()
	FVoxelParameterOverrideCollection_DEPRECATED ParameterCollection_DEPRECATED;

	// 341, removal of UVoxelGraphInstance
	UPROPERTY()
	TObjectPtr<UVoxelParameterContainer_DEPRECATED> ParameterContainer_DEPRECATED;
#endif
};