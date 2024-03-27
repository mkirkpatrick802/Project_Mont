#include "EnemyControllerBase.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

void AEnemyControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

    // Delay OnPossess because it happens before OnBeginPlay
	const float DelayTime = 0.1f;
    FTimerHandle DelayTimerHandle;
    GetWorldTimerManager().SetTimer(DelayTimerHandle, this, &AEnemyControllerBase::InitAIBrain, DelayTime, false);
}

void AEnemyControllerBase::SetEggTarget(const bool TargetEgg)
{
    if(TargetEgg)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), Egg, FoundActors);
        if (FoundActors.Num() > 0)
			GetBlackboardComponent()->SetValueAsObject(FName("Target"), FoundActors[0]);
    }
    else
    {
        GetBlackboardComponent()->SetValueAsObject(FName("Target"), nullptr);
    }
}

void AEnemyControllerBase::InitAIBrain()
{
    if(!BehaviorTree) return;

    RunBehaviorTree(BehaviorTree);
    SetBlackboardData();
}


void AEnemyControllerBase::SetBlackboardData_Implementation() { }