#include "EnemyControllerBase.h"

void AEnemyControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

    // Delay OnPossess because it happens before OnBeginPlay
	const float DelayTime = 0.1f;
    FTimerHandle DelayTimerHandle;
    GetWorldTimerManager().SetTimer(DelayTimerHandle, this, &AEnemyControllerBase::InitAIBrain, DelayTime, false);
}

void AEnemyControllerBase::InitAIBrain()
{
    if(!BehaviorTree) return;

    RunBehaviorTree(BehaviorTree);
    SetBlackboardData();
}


void AEnemyControllerBase::SetBlackboardData_Implementation() { }