// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFutureValue.h"

struct FVoxelNode;
class FVoxelTaskStat;
class FVoxelTaskGroup;
class FVoxelTransformRef;
class FVoxelGraphEvaluator;

extern VOXELGRAPHCORE_API bool GVoxelBypassTaskQueue;

VOXELGRAPHCORE_API bool ShouldExitVoxelTask();
VOXELGRAPHCORE_API bool ShouldRunVoxelTaskInParallel();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VOXELGRAPHCORE_API FVoxelTaskPriority
{
public:
	FVoxelTaskPriority() = default;

	FORCEINLINE static FVoxelTaskPriority MakeTop()
	{
		return FVoxelTaskPriority();
	}
	FORCEINLINE static FVoxelTaskPriority MakeBounds(
		const FVoxelBox& Bounds,
		const double Offset,
		const FObjectKey World,
		const FVoxelTransformRef& LocalToWorld)
	{
		return FVoxelTaskPriority(true, Bounds, Offset, GetPosition(World, LocalToWorld));
	}

	FORCEINLINE double GetPriority() const
	{
		if (!bHasBounds)
		{
			return -1;
		}
		checkVoxelSlow(Position.IsValid());

		const double DistanceSquared = Bounds.ComputeSquaredDistanceFromBoxToPoint(*Position.Get());
		// Keep the sign of Offset
		return DistanceSquared + Offset * FMath::Abs(Offset);
	}

private:
	bool bHasBounds = false;
	FVoxelBox Bounds;
	double Offset = 0;
	TSharedPtr<const FVector> Position;

	FORCEINLINE FVoxelTaskPriority(
		const bool bHasBounds,
		const FVoxelBox& Bounds,
		const double PriorityOffset,
		const TSharedPtr<const FVector>& Position)
		: bHasBounds(bHasBounds)
		, Bounds(Bounds)
		, Offset(PriorityOffset)
		, Position(Position)
	{
	}

	static TSharedRef<const FVector> GetPosition(
		FObjectKey World,
		const FVoxelTransformRef& LocalToWorld);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API FVoxelTaskReferencer
{
public:
	const FName Name;

	explicit FVoxelTaskReferencer(FName Name);

	VOXEL_COUNT_INSTANCES();

	void AddRef(const TSharedRef<const FVoxelVirtualStruct>& Object);
	void AddEvaluator(const TSharedRef<const FVoxelGraphEvaluator>& GraphEvaluator);
	bool IsReferenced(const FVoxelVirtualStruct* Object) const;

	static FVoxelTaskReferencer& Get();

private:
	mutable FVoxelCriticalSection CriticalSection;
	TVoxelSet<TSharedPtr<const FVoxelVirtualStruct>> Objects_RequiresLock;
	TVoxelSet<TSharedPtr<const FVoxelGraphEvaluator>> GraphEvaluators_RequiresLock;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VOXELGRAPHCORE_API FVoxelTask
{
public:
	FVoxelTaskGroup& Group;
	const FName Name;
	const TVoxelUniqueFunction<void()> Lambda;

	FVoxelCounter32 NumDependencies;
	int32 PendingTaskIndex = -1;

	FVoxelTask(
		FVoxelTaskGroup& Group,
		const FName Name,
		TVoxelUniqueFunction<void()> Lambda)
		: Group(Group)
		, Name(Name)
		, Lambda(MoveTemp(Lambda))
	{
	}
	UE_NONCOPYABLE(FVoxelTask);

	VOXEL_COUNT_INSTANCES();

	FORCEINLINE void Execute() const
	{
#if VOXEL_DEBUG
		CheckIsSafeToExecute();
#endif
		Lambda();
	}

private:
#if VOXEL_DEBUG
	void CheckIsSafeToExecute() const;
#endif
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API FVoxelTaskFactory
{
public:
	FVoxelTaskFactory() = default;

	FORCEINLINE FVoxelTaskFactory& RunOnGameThread()
	{
		ensureVoxelSlow(!bPrivateRunOnGameThread);
		bPrivateRunOnGameThread = true;
		return *this;
	}
	FORCEINLINE FVoxelTaskFactory& Dependency(const FVoxelFutureValue& Dependency)
	{
		ensureVoxelSlow(DependenciesView.Num() == 0);
		DependenciesView = MakeVoxelArrayView(Dependency);
		return *this;
	}
	FORCEINLINE FVoxelTaskFactory& Dependencies(const TConstVoxelArrayView<FVoxelFutureValue> Dependencies)
	{
		ensureVoxelSlow(DependenciesView.Num() == 0);
		DependenciesView = Dependencies;
		return *this;
	}
	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FVoxelFutureValue>::Value>>
	FORCEINLINE FVoxelTaskFactory& Dependencies(const TVoxelArray<T>& Dependencies)
	{
		ensureVoxelSlow(DependenciesView.Num() == 0);
		DependenciesView = ReinterpretCastVoxelArrayView<FVoxelFutureValue>(MakeVoxelArrayView(Dependencies));
		return *this;
	}
	template<typename... ArgTypes, typename = std::enable_if_t<TAnd<TIsDerivedFrom<ArgTypes, FVoxelFutureValue>...>::Value>>
	FORCEINLINE FVoxelTaskFactory& Dependencies(const ArgTypes&... Args)
	{
		ensureVoxelSlow(InlineDependencies.Num() == 0);
		VOXEL_FOLD_EXPRESSION(InlineDependencies.Add(Args));
		return Dependencies(InlineDependencies);
	}

	void Execute(TVoxelUniqueFunction<void()> Lambda);
	FVoxelFutureValue Execute(const FVoxelPinType& Type, TVoxelUniqueFunction<FVoxelFutureValue()>&& Lambda);

	template<typename T>
	TVoxelFutureValue<T> Execute(TVoxelUniqueFunction<TVoxelFutureValue<T>()>&& Lambda)
	{
		return this->Execute<T>(FVoxelPinType::Make<T>(), MoveTemp(Lambda));
	}
	template<typename T>
	TVoxelFutureValue<T> Execute(const FVoxelPinType& Type, TVoxelUniqueFunction<TVoxelFutureValue<T>()>&& Lambda)
	{
		checkVoxelSlow(Type.CanBeCastedTo<T>());
		return TVoxelFutureValue<T>(this->Execute(Type, ReinterpretCastRef<TVoxelUniqueFunction<FVoxelFutureValue()>>(MoveTemp(Lambda))));
	}

private:
	FName PrivateName;
	bool bPrivateRunOnGameThread = false;
	TConstVoxelArrayView<FVoxelFutureValue> DependenciesView;
	TVoxelInlineArray<FVoxelFutureValue, 8> InlineDependencies;

	TVoxelUniqueFunction<void()> MakeStatsLambda(TVoxelUniqueFunction<void()> Lambda) const;
	static TVoxelUniqueFunction<void()> MakeGameThreadLambda(TVoxelUniqueFunction<void()> Lambda);

	friend FVoxelTaskFactory MakeVoxelTaskImpl(FName);
};

FORCEINLINE FVoxelTaskFactory MakeVoxelTaskImpl(const FName Name)
{
	FVoxelTaskFactory Factory;
	Factory.PrivateName = Name;
	return Factory;
}

VOXELGRAPHCORE_API FString GetVoxelTaskName(
	const FString& FunctionName,
	int32 Line,
	const FString& Context);

#define MakeVoxelTask(...) MakeVoxelTaskImpl(STATIC_FNAME(GetVoxelTaskName(__FUNCTION__, __LINE__, FString(__VA_ARGS__))))

#if INTELLISENSE_PARSER
#undef MakeVoxelTask
FVoxelTaskFactory MakeVoxelTask(const char* Name = {});
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern VOXELGRAPHCORE_API const uint32 GVoxelTaskTagTLS;

struct VOXELGRAPHCORE_API FVoxelTaskTagScope
{
public:
	FORCEINLINE static TVoxelUniquePtr<FVoxelTaskTagScope> Create(const FName TagName)
	{
		if (!AreVoxelStatsEnabled())
		{
			return nullptr;
		}
		return CreateImpl(TagName);
	}
	~FVoxelTaskTagScope();
	UE_NONCOPYABLE(FVoxelTaskTagScope);

	TVoxelArray<FName> GetTagNames() const;

public:
	FORCEINLINE static FVoxelTaskTagScope* Get()
	{
		return static_cast<FVoxelTaskTagScope*>(FPlatformTLS::GetTlsValue(GVoxelTaskTagTLS));
	}

private:
	FName TagName;
	TOptional<TVoxelArray<FName>> TagNameOverrides;
	FVoxelTaskTagScope* ParentScope = nullptr;

	static TVoxelUniquePtr<FVoxelTaskTagScope> CreateImpl(FName TagName);
	FVoxelTaskTagScope();

	friend FVoxelTaskFactory;
};

#define VOXEL_TASK_TAG_SCOPE(Name) \
	const TVoxelUniquePtr<FVoxelTaskTagScope> VOXEL_APPEND_LINE(Scope) = FVoxelTaskTagScope::Create(STATIC_FNAME(Name));