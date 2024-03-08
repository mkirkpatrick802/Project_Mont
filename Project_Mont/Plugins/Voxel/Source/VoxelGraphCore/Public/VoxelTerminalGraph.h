// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinValue.h"
#include "VoxelTerminalGraph.generated.h"

class UVoxelGraph;
class UVoxelEdGraph;
class UVoxelTerminalGraphRuntime;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelGraphMetadata
{
	GENERATED_BODY()

	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	FString Category;

	UPROPERTY()
	FString Description;
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelGraphProperty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Config")
	FName Name;

	UPROPERTY(EditAnywhere, Category = "Config")
	FVoxelPinType Type;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Config")
	FString Category;

	UPROPERTY(EditAnywhere, Category = "Config")
	FString Description;
#endif
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelGraphInput : public FVoxelGraphProperty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Default Value")
	FVoxelPinValue DefaultValue;

	UPROPERTY()
	bool bDeprecatedExposeInputDefaultAsPin = false;

	void Fixup();
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelGraphOutput : public FVoxelGraphProperty
{
	GENERATED_BODY()
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelGraphLocalVariable : public FVoxelGraphProperty
{
	GENERATED_BODY()

	UPROPERTY()
	FVoxelPinValue DeprecatedDefaultValue;
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelGraphPreviewConfig
{
	GENERATED_BODY()

	UPROPERTY()
	EVoxelAxis Axis = EVoxelAxis::Z;

	UPROPERTY()
	int32 Resolution = 512;

	UPROPERTY()
	FVector Position = FVector::ZeroVector;

	UPROPERTY()
	double Zoom = 1.;

	float GetAxisLocation() const
	{
		switch (Axis)
		{
		default: return 0.f;
		case EVoxelAxis::X: return Position.X;
		case EVoxelAxis::Y: return Position.Y;
		case EVoxelAxis::Z: return Position.Z;
		}
	}
	void SetAxisLocation(const float Value)
	{
		switch (Axis)
		{
		default: ensure(false);
		case EVoxelAxis::X: Position.X = Value; break;
		case EVoxelAxis::Y: Position.Y = Value; break;
		case EVoxelAxis::Z: Position.Z = Value; break;
		}
	}
};

UCLASS(Within=VoxelGraph)
class VOXELGRAPHCORE_API UVoxelTerminalGraph : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	// Only used if this is in a function library
	UPROPERTY(EditAnywhere, Category = "Config")
	bool bExposeToLibrary = true;

	UPROPERTY()
	FVoxelGraphPreviewConfig PreviewConfig;
#endif

public:
	UVoxelGraph& GetGraph();
	const UVoxelGraph& GetGraph() const;

	FGuid GetGuid() const;
	void SetGuid_Hack(const FGuid& Guid);

#if WITH_EDITOR
	FString GetDisplayName() const;
	FVoxelGraphMetadata GetMetadata() const;
	void UpdateMetadata(TFunctionRef<void(FVoxelGraphMetadata&)> Lambda);
#endif

#if WITH_EDITOR
	void SetEdGraph_Hack(UEdGraph* NewEdGraph);
	void SetDisplayName_Hack(const FString& Name);

	UEdGraph& GetEdGraph();
	const UEdGraph& GetEdGraph() const;

	template<typename T = UVoxelEdGraph>
	T& GetTypedEdGraph()
	{
		return *CastChecked<T>(&GetEdGraph());
	}
	template<typename T = UVoxelEdGraph>
	const T& GetTypedEdGraph() const
	{
		return *CastChecked<T>(&GetEdGraph());
	}
#endif

public:
	const FVoxelGraphInput* FindInput(const FGuid& Guid) const;
	const FVoxelGraphOutput* FindOutput(const FGuid& Guid) const;
	const FVoxelGraphOutput* FindOutputByName(const FName& Name, FGuid& OutGuid) const;
	const FVoxelGraphLocalVariable* FindLocalVariable(const FGuid& Guid) const;

	const FVoxelGraphInput& FindInputChecked(const FGuid& Guid) const;
	const FVoxelGraphOutput& FindOutputChecked(const FGuid& Guid) const;
	const FVoxelGraphLocalVariable& FindLocalVariableChecked(const FGuid& Guid) const;

	TVoxelSet<FGuid> GetInputs() const;
	TVoxelSet<FGuid> GetOutputs() const;
	TVoxelSet<FGuid> GetLocalVariables() const;

	bool IsInheritedInput(const FGuid& Guid) const;
	bool IsInheritedOutput(const FGuid& Guid) const;

public:
#if WITH_EDITOR
	void AddInput(const FGuid& Guid, const FVoxelGraphInput& Input);
	void AddOutput(const FGuid& Guid, const FVoxelGraphOutput& Output);
	void AddLocalVariable(const FGuid& Guid, const FVoxelGraphLocalVariable& LocalVariable);

	void RemoveInput(const FGuid& Guid);
	void RemoveOutput(const FGuid& Guid);
	void RemoveLocalVariable(const FGuid& Guid);

	void UpdateInput(const FGuid& Guid, TFunctionRef<void(FVoxelGraphInput&)> Update);
	void UpdateOutput(const FGuid& Guid, TFunctionRef<void(FVoxelGraphOutput&)> Update);
	void UpdateLocalVariable(const FGuid& Guid, TFunctionRef<void(FVoxelGraphLocalVariable&)> Update);

	void ReorderInputs(TConstVoxelArrayView<FGuid> NewGuids);
	void ReorderOutputs(TConstVoxelArrayView<FGuid> NewGuids);
	void ReorderLocalVariables(TConstVoxelArrayView<FGuid> NewGuids);
#endif

private:
	UPROPERTY()
	FGuid PrivateGuid;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FVoxelGraphMetadata PrivateMetadata;
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UEdGraph> EdGraph;
#endif

	UPROPERTY()
	TMap<FGuid, FVoxelGraphInput> GuidToInput;

	UPROPERTY()
	TMap<FGuid, FVoxelGraphOutput> GuidToOutput;

	UPROPERTY()
	TMap<FGuid, FVoxelGraphLocalVariable> GuidToLocalVariable;

public:
	UVoxelTerminalGraph();

	void Fixup();
	bool HasExecNodes() const;
	bool IsMainTerminalGraph() const;
	// Isn't an inherited terminal graph
	bool IsTopmostTerminalGraph() const;

	FORCEINLINE UVoxelTerminalGraphRuntime& GetRuntime() const
	{
		return *Runtime;
	}

public:
	// Includes self
	TVoxelInlineSet<UVoxelTerminalGraph*, 8> GetBaseTerminalGraphs();
	TVoxelInlineSet<const UVoxelTerminalGraph*, 8> GetBaseTerminalGraphs() const;

	// Includes self, slow
	TVoxelSet<UVoxelTerminalGraph*> GetChildTerminalGraphs_LoadedOnly();
	TVoxelSet<const UVoxelTerminalGraph*> GetChildTerminalGraphs_LoadedOnly() const;

public:
	//~ Begin UObject Interface
	virtual void PostLoad() override;
	//~ End UObject Interface

private:
	UPROPERTY()
	TObjectPtr<UVoxelTerminalGraphRuntime> Runtime;
};