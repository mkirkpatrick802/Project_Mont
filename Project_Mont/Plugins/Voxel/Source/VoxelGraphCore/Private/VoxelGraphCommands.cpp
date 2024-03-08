// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.h"
#include "VoxelActor.h"
#include "VoxelSettings.h"
#include "VoxelComponent.h"
#include "VoxelActorBase.h"
#include "VoxelNodeStats.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#include "VoxelMovingAverageBuffer.h"
#include "Material/VoxelMaterialDefinition.h"
#if WITH_EDITOR
#include "MessageLogModule.h"
#include "IMessageLogListing.h"
#include "Editor/EditorEngine.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#endif

TSet<FObjectKey> GVoxelObjectsDestroyedByFrameRateLimit;

VOXEL_CONSOLE_COMMAND(
	RefreshAll,
	"voxel.RefreshAll",
	"")
{
#if WITH_EDITOR
	if (GEditor)
	{
		FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog").GetLogListing("Voxel")->ClearMessages();
	}

	for (IVoxelNodeStatProvider* Provider : GVoxelNodeStatProviders)
	{
		Provider->ClearStats();
	}
#endif

#if WITH_EDITOR
	for (const UVoxelTerminalGraph* TerminalGraph : TObjectRange<UVoxelTerminalGraph>())
	{
		if (TerminalGraph->IsTemplate())
		{
			continue;
		}

		GVoxelGraphTracker->NotifyEdGraphChanged(TerminalGraph->GetEdGraph());
	}
#endif

	ForEachObjectOfClass<AVoxelActorBase>([&](AVoxelActorBase& Actor)
	{
		if (Actor.IsRuntimeCreated() ||
			GVoxelObjectsDestroyedByFrameRateLimit.Contains(&Actor))
		{
			Actor.QueueRecreate();
		}
	});

	checkStatic(!TIsDerivedFrom<AVoxelActor, AVoxelActorBase>::Value);

	ForEachObjectOfClass<AVoxelActor>([&](AVoxelActor& Actor)
	{
		if (Actor.IsRuntimeCreated() ||
			GVoxelObjectsDestroyedByFrameRateLimit.Contains(&Actor))
		{
			Actor.QueueRecreate();
		}
	});

	ForEachObjectOfClass<UVoxelComponent>([&](UVoxelComponent& Component)
	{
		if (Component.IsRuntimeCreated() ||
			GVoxelObjectsDestroyedByFrameRateLimit.Contains(&Component))
		{
			Component.QueueRecreate();
		}
	});

	ForEachObjectOfClass<UVoxelMaterialDefinition>([&](UVoxelMaterialDefinition& Definition)
	{
		Definition.QueueRebuildTextures();
	});

	GVoxelObjectsDestroyedByFrameRateLimit.Empty();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static TArray<FString> ParseArguments(const TArray<FString>& Args)
{
	TArray<FString> Result;

	FString CurrentArg;
	for (int32 Index = 0; Index < Args.Num(); Index++)
	{
		if (Args[Index].StartsWith("\""))
		{
			CurrentArg = Args[Index];
			CurrentArg.RemoveFromStart("\"");

			if (CurrentArg.EndsWith("\""))
			{
				CurrentArg.RemoveFromEnd("\"");
				Result.Add(CurrentArg);
			}
			continue;
		}

		if (CurrentArg.IsEmpty())
		{
			Result.Add(Args[Index]);
			continue;
		}

		if (!Args[Index].EndsWith("\""))
		{
			CurrentArg += " " + Args[Index];
			continue;
		}

		CurrentArg += " " + Args[Index];
		CurrentArg.RemoveFromEnd("\"");
		Result.Add(CurrentArg);
		CurrentArg = {};
	}

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VOXEL_CONSOLE_WORLD_COMMAND(
	GetParameter,
	"voxel.get",
	"[ActorName] [ParameterName] Gets graph parameter value from specific actor if specified, all voxel actors otherwise. Actor name is optional.")
{
	if (Args.Num() == 0)
	{
		UE_LOG(LogConsoleResponse, Warning, TEXT("Missing parameter name argument"));
		return;
	}

	TArray<FString> ParsedArgs = ParseArguments(Args);

	FString ActorName;
	FName ParameterName;
	if (ParsedArgs.Num() > 1)
	{
		ActorName = ParsedArgs[0];
		ParameterName = FName(ParsedArgs[1]);
	}
	else
	{
		ParameterName = FName(ParsedArgs[0]);
	}

	const auto PrintParameter = [&](
		const IVoxelParameterOverridesObjectOwner& Owner,
		const FString& DebugName,
		const bool bPrintError)
	{
		if (!Owner.HasParameter(ParameterName))
		{
			if (bPrintError)
			{
				UE_LOG(LogConsoleResponse, Warning, TEXT("%s: No parameter %s found"), *DebugName, *ParameterName.ToString());
			}
			return;
		}

		const FVoxelPinValue Value = Owner.GetParameter(ParameterName);
		if (!ensure(Value.IsValid()))
		{
			return;
		}

		UE_LOG(LogConsoleResponse, Log, TEXT("%s.%s=%s"), *DebugName, *ParameterName.ToString(), *Value.ExportToString());
	};

	if (ActorName.IsEmpty())
	{
		for (const AVoxelActor* Actor : TActorRange<AVoxelActor>(World))
		{
			PrintParameter(*Actor, Actor->GetActorNameOrLabel(), false);
		}

		for (const UVoxelComponent* Component : TObjectRange<UVoxelComponent>())
		{
			if (Component->GetWorld() != World)
			{
				continue;
			}

			PrintParameter(*Component, Component->GetOwner()->GetActorNameOrLabel(), false);
		}
		return;
	}

	for (const AVoxelActor* Actor : TActorRange<AVoxelActor>(World))
	{
		if (!ensure(Actor) ||
			Actor->GetActorNameOrLabel() != ActorName)
		{
			continue;
		}

		PrintParameter(*Actor, Actor->GetActorNameOrLabel(), true);
		return;
	}

	for (const UVoxelComponent* Component : TObjectRange<UVoxelComponent>())
	{
		if (Component->GetWorld() != World ||
			Component->GetOwner()->GetActorNameOrLabel() != ActorName)
		{
			continue;
		}

		PrintParameter(*Component, Component->GetOwner()->GetActorNameOrLabel(), true);
		return;
	}

	UE_LOG(LogConsoleResponse, Warning, TEXT("No actor %s found"), *ActorName);
}

VOXEL_CONSOLE_WORLD_COMMAND(
	SetParameter,
	"voxel.set",
	"[ActorName] [ParameterName] [Value] Sets graph parameter value for either specific voxel actor if specified, otherwise all voxel actors. Actor name is optional.")
{
	TArray<FString> ParsedArgs = ParseArguments(Args);
	if (ParsedArgs.Num() < 2)
	{
		UE_LOG(LogConsoleResponse, Warning, TEXT("Missing arguments"));
		return;
	}

	FString ActorName;
	FName ParameterName;
	FString ParameterValue;
	if (ParsedArgs.Num() > 2)
	{
		ActorName = ParsedArgs[0];
		ParameterName = FName(ParsedArgs[1]);
		ParameterValue = ParsedArgs[2];
	}
	else
	{
		ParameterName = FName(ParsedArgs[0]);
		ParameterValue = ParsedArgs[1];
	}

	const auto SetParameter = [&](
		IVoxelParameterOverridesObjectOwner& Owner,
		const FString& DebugName,
		const bool bPrintError)
	{
		if (!Owner.HasParameter(ParameterName))
		{
			if (bPrintError)
			{
				UE_LOG(LogConsoleResponse, Warning, TEXT("%s: No parameter %s found"), *DebugName, *ParameterName.ToString());
			}
			return;
		}

		FVoxelPinValue Value = Owner.GetParameter(ParameterName);
		if (!Value.IsValid())
		{
			return;
		}

		if (!Value.ImportFromString(ParameterValue))
		{
			UE_LOG(LogConsoleResponse, Warning, TEXT("%s: Failed to set %s=%s"), *DebugName, *ParameterName.ToString(), *ParameterValue);
			return;
		}

		FString Error;
		if (!Owner.SetParameter(ParameterName, Value, &Error))
		{
			UE_LOG(LogConsoleResponse, Warning, TEXT("%s: %s"), *DebugName, *Error);
			return;
		}

		UE_LOG(LogConsoleResponse, Log, TEXT("%s.%s=%s"), *DebugName, *ParameterName.ToString(), *Value.ExportToString());
	};

	if (ActorName.IsEmpty())
	{
		for (AVoxelActor* Actor : TActorRange<AVoxelActor>(World))
		{
			SetParameter(*Actor, Actor->GetActorNameOrLabel(), false);
		}

		for (UVoxelComponent* Component : TObjectRange<UVoxelComponent>())
		{
			if (Component->GetWorld() != World)
			{
				continue;
			}

			SetParameter(*Component, Component->GetOwner()->GetActorNameOrLabel(), false);
		}
		return;
	}

	for (AVoxelActor* Actor : TActorRange<AVoxelActor>(World))
	{
		if (Actor->GetActorNameOrLabel() != ActorName)
		{
			continue;
		}

		SetParameter(*Actor, Actor->GetActorNameOrLabel(), true);
		return;
	}

	for (UVoxelComponent* Component : TObjectRange<UVoxelComponent>())
	{
		if (Component->GetWorld() != World ||
			Component->GetOwner()->GetActorNameOrLabel() != ActorName)
		{
			continue;
		}

		SetParameter(*Component, Component->GetOwner()->GetActorNameOrLabel(), true);
		return;
	}

	UE_LOG(LogConsoleResponse, Warning, TEXT("No actor %s found"), *ActorName);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
class FVoxelSafetyTicker : public FVoxelTicker
{
public:
	//~ Begin FVoxelTicker Interface
	virtual void Tick() override
	{
		VOXEL_FUNCTION_COUNTER();

		if (!GetDefault<UVoxelSettings>()->bEnablePerformanceMonitoring)
		{
			return;
		}

		if (GEditor->ShouldThrottleCPUUsage() ||
			GEditor->PlayWorld ||
			GIsPlayInEditorWorld)
		{
			// Don't check framerate when throttling or in PIE
			return;
		}

		const int32 FramesToAverage = FMath::Max(2, GetDefault<UVoxelSettings>()->FramesToAverage);
		if (FramesToAverage != Buffer.GetWindowSize())
		{
			Buffer = FVoxelMovingAverageBuffer(FramesToAverage);
		}

		// Avoid outliers (typically, debugger breaking) causing a huge average
		const double SanitizedDeltaTime = FMath::Clamp(FApp::GetDeltaTime(), 0.001, 1);
		Buffer.AddValue(SanitizedDeltaTime);

		if (1.f / Buffer.GetAverageValue() > GetDefault<UVoxelSettings>()->MinFPS)
		{
			bDestroyedRuntimes = false;
			return;
		}

		if (bDestroyedRuntimes)
		{
			return;
		}
		bDestroyedRuntimes = true;

		FNotificationInfo Info(INVTEXT("Average framerate is below 8fps, destroying all voxel runtimes. Use Ctrl F5 to re-create them"));
		Info.ExpireDuration = 4.f;
		Info.ButtonDetails.Add(FNotificationButtonInfo(
			INVTEXT("Disable Monitoring"),
			INVTEXT("Disable framerate monitoring"),
			MakeLambdaDelegate([]
			{
				GetMutableDefault<UVoxelSettings>()->bEnablePerformanceMonitoring = false;
				GetMutableDefault<UVoxelSettings>()->PostEditChange();
			}),
			SNotificationItem::CS_None));
		FSlateNotificationManager::Get().AddNotification(Info);

		ForEachObjectOfClass_Copy<AVoxelActorBase>([&](AVoxelActorBase& Actor)
		{
			if (!Actor.IsRuntimeCreated())
			{
				return;
			}

			if (!ensure(Actor.GetWorld()) ||
				!Actor.GetWorld()->IsEditorWorld())
			{
				return;
			}

			Actor.Destroy();
			GVoxelObjectsDestroyedByFrameRateLimit.Add(&Actor);
		});

		ForEachObjectOfClass_Copy<AVoxelActor>([&](AVoxelActor& Actor)
		{
			if (!Actor.IsRuntimeCreated())
			{
				return;
			}

			if (!ensure(Actor.GetWorld()) ||
				!Actor.GetWorld()->IsEditorWorld())
			{
				return;
			}

			Actor.DestroyRuntime();
			GVoxelObjectsDestroyedByFrameRateLimit.Add(&Actor);
		});

		ForEachObjectOfClass_Copy<UVoxelComponent>([&](UVoxelComponent& Component)
		{
			if (!Component.IsRuntimeCreated())
			{
				return;
			}

			if (!ensure(Component.GetWorld()) ||
				!Component.GetWorld()->IsEditorWorld())
			{
				return;
			}

			Component.DestroyRuntime();
			GVoxelObjectsDestroyedByFrameRateLimit.Add(&Component);
		});
	}
	//~ End FVoxelTicker Interface

private:
	FVoxelMovingAverageBuffer Buffer = FVoxelMovingAverageBuffer(2);
	bool bDestroyedRuntimes = false;
};

VOXEL_RUN_ON_STARTUP_EDITOR(CreateVoxelSafetyTicker)
{
	if (!GEditor)
	{
		return;
	}

	new FVoxelSafetyTicker();
}
#endif