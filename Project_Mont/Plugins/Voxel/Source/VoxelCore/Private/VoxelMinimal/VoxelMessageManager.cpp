// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.h"
#include "VoxelMessage.h"
#include "Logging/MessageLog.h"
#include "Framework/Application/SlateApplication.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif

FVoxelMessageManager* GVoxelMessageManager = new FVoxelMessageManager();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelMessagesThreadSingleton : public TThreadSingleton<FVoxelMessagesThreadSingleton>
{
public:
	// Weak ptr because messages are built on the game thread
	TArray<TWeakPtr<IVoxelMessageConsumer>> MessageConsumers;

	TWeakPtr<IVoxelMessageConsumer> GetTop() const
	{
		if (MessageConsumers.Num() == 0)
		{
			return nullptr;
		}
		return MessageConsumers.Last();
	}
};

FVoxelScopedMessageConsumer::FVoxelScopedMessageConsumer(TWeakPtr<IVoxelMessageConsumer> MessageConsumer)
{
	FVoxelMessagesThreadSingleton::Get().MessageConsumers.Add(MessageConsumer);
}

FVoxelScopedMessageConsumer::FVoxelScopedMessageConsumer(TFunction<void(const TSharedRef<FVoxelMessage>&)> LogMessage)
{
	class FMessageConsumer : public IVoxelMessageConsumer
	{
	public:
		TFunction<void(const TSharedRef<FVoxelMessage>&)> LogMessageLambda;

		virtual void LogMessage(const TSharedRef<FVoxelMessage>& Message) override
		{
			LogMessageLambda(Message);
		}
	};

	const TSharedRef<FMessageConsumer> Consumer = MakeVoxelShared<FMessageConsumer>();
	Consumer->LogMessageLambda = LogMessage;

	TempConsumer = Consumer;
	FVoxelMessagesThreadSingleton::Get().MessageConsumers.Add(Consumer);
}

FVoxelScopedMessageConsumer::~FVoxelScopedMessageConsumer()
{
	FVoxelMessagesThreadSingleton::Get().MessageConsumers.Pop(false);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMessageManager::LogMessage(const TSharedRef<FVoxelMessage>& Message) const
{
	VOXEL_FUNCTION_COUNTER();

	for (const FGatherCallstack& GatherCallstack : GatherCallstacks)
	{
		GatherCallstack(Message);
	}

	const TSharedPtr<IVoxelMessageConsumer> MessageConsumer = FVoxelMessagesThreadSingleton::Get().GetTop().Pin();

	RunOnGameThread([=]
	{
		VOXEL_SCOPE_COUNTER("Log");

		if (MessageConsumer)
		{
			MessageConsumer->LogMessage(Message);
		}
		else
		{
			LogMessage_GameThread(Message);
		}
	});
}

void FVoxelMessageManager::LogMessage_GameThread(const TSharedRef<FVoxelMessage>& Message) const
{
	VOXEL_FUNCTION_COUNTER();

	if (NO_LOGGING)
	{
		return;
	}

	{
		VOXEL_SCOPE_COUNTER("Check recent messages");

		struct FRecentMessage
		{
			uint64 Hash = 0;
			double Time = 0;
		};
		static TCompatibleVoxelArray<FRecentMessage> RecentMessages;

		const uint64 Hash = Message->GetHash();
		const double Time = FPlatformTime::Seconds();

		RecentMessages.RemoveAllSwap([&](const FRecentMessage& RecentMessage)
		{
			return RecentMessage.Time + 0.5 < Time;
		}, false);

		if (RecentMessages.FindByPredicate([&](const FRecentMessage& RecentMessage)
			{
				return RecentMessage.Hash == Hash;
			}))
		{
			// A message with the same hash was displayed recently, don't log this one again
			return;
		}

		RecentMessages.Add(FRecentMessage
		{
			Hash,
			Time
		});
	}

	if (!GIsEditor)
	{
		switch (Message->GetSeverity())
		{
		default: VOXEL_ASSUME(false);
		case EVoxelMessageSeverity::Info:
		{
			LOG_VOXEL(Log, "%s", *Message->ToString());
		}
		break;
		case EVoxelMessageSeverity::Warning:
		{
			LOG_VOXEL(Warning, "%s", *Message->ToString());
		}
		break;
		case EVoxelMessageSeverity::Error:
		{
			if (IsRunningCookCommandlet() ||
				IsRunningCookOnTheFly())
			{
				// Don't fail cooking
				LOG_VOXEL(Warning, "%s", *Message->ToString());
			}
			else
			{
				LOG_VOXEL(Error, "%s", *Message->ToString());
			}
		}
		break;
		}

		OnMessageLogged.Broadcast(Message);
		return;
	}

#if WITH_EDITOR
	const auto LogMessage = [=]
	{
		OnMessageLogged.Broadcast(Message);

		const TSharedRef<FTokenizedMessage> TokenizedMessage = Message->CreateTokenizedMessage();

		if (GEditor->PlayWorld ||
			GIsPlayInEditorWorld)
		{
			FMessageLog("PIE").AddMessage(TokenizedMessage);
		}

		FMessageLog("Voxel").AddMessage(TokenizedMessage);
	};

	if (FSlateApplication::IsInitialized() &&
		FSlateApplication::Get().GetActiveModalWindow())
	{
		// DelayedCall would only run once the modal is closed
		LogMessage();
		return;
	}

	FVoxelUtilities::DelayedCall(LogMessage);
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMessageManager::LogMessageFormat(const EVoxelMessageSeverity Severity, const TCHAR* Format)
{
	if (NO_LOGGING)
	{
		return;
	}

	GVoxelMessageManager->InternalLogMessageFormat(Severity, Format, {});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMessageManager::InternalLogMessageFormat(
	const EVoxelMessageSeverity Severity,
	const TCHAR* Format,
	const TConstVoxelArrayView<TSharedRef<FVoxelMessageToken>> Tokens) const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<bool> UsedTokens;
	UsedTokens.SetNumZeroed(Tokens.Num());

	const TSharedRef<FVoxelMessage> Message = FVoxelMessage::Create(Severity);

	while (Format)
	{
		// Read to next "{":
		const TCHAR* Delimiter = FCString::Strstr(Format, TEXT("{"));
		if (!Delimiter)
		{
			// Add the remaining text
			const FString RemainingText(Format);
			if (!RemainingText.IsEmpty())
			{
				Message->AddText(RemainingText);
			}
			break;
		}

		// Add the text left of the {
		const FString TextBefore(Delimiter - Format, Format);
		if (!TextBefore.IsEmpty())
		{
			Message->AddText(TextBefore);
		}

		Format = Delimiter + FCString::Strlen(TEXT("{"));

		// Read to next "}":
		Delimiter = FCString::Strstr(Format, TEXT("}"));
		if (!ensureMsgf(Delimiter, TEXT("Missing }")))
		{
			break;
		}

		const FString IndexString(Delimiter - Format, Format);
		if (!ensure(FCString::IsNumeric(*IndexString)))
		{
			break;
		}

		const int32 Index = FCString::Atoi(*IndexString);
		if (!ensureMsgf(Tokens.IsValidIndex(Index), TEXT("Out of bound index: {%d}"), Index))
		{
			break;
		}

		UsedTokens[Index] = true;
		Message->AddToken(Tokens[Index]);

		Format = Delimiter + FCString::Strlen(TEXT("}"));
	}

	for (int32 Index = 0; Index < Tokens.Num(); Index++)
	{
		ensureMsgf(UsedTokens[Index], TEXT("Unused arg: %d"), Index);
	}

	LogMessage(Message);
}