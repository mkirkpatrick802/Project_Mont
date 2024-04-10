#include "Project_MontGameModeBase.h"

void AProject_MontGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle TimerHandle;
	const float RandomInterval = FMath::RandRange(EggActivationMinimum, EggActivationMaximum);
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AProject_MontGameModeBase::ActivateEgg, RandomInterval, true);
}

void AProject_MontGameModeBase::ActivateEgg()
{
	if (CurrentEggState) return;
	CurrentEggState = true;

	UE_LOG(LogTemp, Warning, TEXT("--- Egg Activated"));
	EggStateChanged.Broadcast(CurrentEggState);

	FTimerHandle TimerHandle;
	const float RandomInterval = FMath::RandRange(EggDeactivationMinimum, EggDeactivationMaximum);
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AProject_MontGameModeBase::DeactivateEgg, RandomInterval, false);
}

void AProject_MontGameModeBase::DeactivateEgg()
{
	if(!CurrentEggState) return;
	CurrentEggState = false;

	UE_LOG(LogTemp, Warning, TEXT("--- Egg Deactivated"));
	EggStateChanged.Broadcast(CurrentEggState);
}
