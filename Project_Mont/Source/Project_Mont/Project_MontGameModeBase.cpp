#include "Project_MontGameModeBase.h"

void AProject_MontGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle TimerHandle;
	const float RandomInterval = FMath::RandRange(80.f, 180.f);
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AProject_MontGameModeBase::ActivateEgg, RandomInterval, true);
}

void AProject_MontGameModeBase::ActivateEgg() const
{
	UE_LOG(LogTemp, Warning, TEXT("--- Egg Activated"));
	EggStateChanged.Broadcast(true);

	FTimerHandle TimerHandle;
	const float RandomInterval = FMath::RandRange(30.f, 40.f);
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AProject_MontGameModeBase::DeactivateEgg, RandomInterval, false);
}

void AProject_MontGameModeBase::DeactivateEgg() const
{
	UE_LOG(LogTemp, Warning, TEXT("--- Egg Deactivated"));
	EggStateChanged.Broadcast(false);
}
