#include "EnemyControllerBase.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Egg.h"
#include "EnemyCharacterBase.h"

void AEnemyControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

    // Delay OnPossess because it happens before OnBeginPlay
	const float DelayTime = 0.1f;
    FTimerHandle DelayTimerHandle;
    GetWorldTimerManager().SetTimer(DelayTimerHandle, this, &AEnemyControllerBase::InitAIBrain, DelayTime, false);
}

void AEnemyControllerBase::SetEggTarget(const bool ShouldTargetEgg)
{
    if(ShouldTargetEgg)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), Egg, FoundActors);
        if (FoundActors.Num() > 0)
            TargetEgg = Cast<AEgg>(FoundActors[0]);
    }
    else
    {
        TargetEgg = nullptr;
    }

    GetBlackboardComponent()->SetValueAsObject(FName("Target"), TargetEgg);
}

void AEnemyControllerBase::InitAIBrain()
{
    if(!BehaviorTree) return;

    RunBehaviorTree(BehaviorTree);
    SetBlackboardData();

    ControlledEnemy = Cast<AEnemyCharacterBase>(GetPawn());
}

void AEnemyControllerBase::SetBlackboardData_Implementation() { }