#include "EnemyControllerBase.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Egg.h"
#include "EnemyCharacterBase.h"

AEnemyControllerBase::AEnemyControllerBase()
{

}

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
    if (!BehaviorTree) return;

    RunBehaviorTree(BehaviorTree);
    SetBlackboardData();

    ControlledEnemy = Cast<AEnemyCharacterBase>(GetPawn());
}

void AEnemyControllerBase::SetEggTarget(const bool ShouldTargetEgg)
{
    if(ShouldTargetEgg)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), Egg, FoundActors);
        if (FoundActors.Num() > 0)
            TargetEgg = Cast<AEgg>(FoundActors[0]);

        SetCurrentTargetState(ETargetState::TS_Egg);
    }
    else
    {
        TargetEgg = nullptr;

        SetCurrentTargetState(ETargetState::TS_Update);
    }
}

void AEnemyControllerBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

    if (!TargetEgg || !ControlledEnemy->SeenPlayer)
    {
        GetWorldTimerManager().ClearTimer(DistanceCheckHandle);
        ClosestTarget = nullptr;
        return;
    }

    if(!DistanceCheckHandle.IsValid())
        GetWorldTimerManager().SetTimer(DistanceCheckHandle, this, &AEnemyControllerBase::DistanceCheck, 1, true);
}

void AEnemyControllerBase::DistanceCheck()
{
	const AActor* LastClosestTarget = ClosestTarget;
    ClosestTarget = FindClosestActor(TargetEgg, ControlledEnemy->SeenPlayer);

    if (LastClosestTarget != ClosestTarget)
        SetCurrentTargetState(ETargetState::TS_Update);

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(100, 5, FColor::Red, FString::Printf(TEXT("Closest Player: %s"), *ClosestTarget->GetName()));
}

void AEnemyControllerBase::SetCurrentTargetState(const ETargetState NewTargetState)
{
    AActor* Target = nullptr;

    switch (NewTargetState)
    {
    case ETargetState::TS_Player:

        if (TargetEgg)
        {
            DistanceCheck();

            Target = ClosestTarget;
            CurrentTargetState = (Target == TargetEgg) ? ETargetState::TS_Egg : ETargetState::TS_Player;
        }
        else
        {
            CurrentTargetState = ETargetState::TS_Player;
            Target = ControlledEnemy->SeenPlayer;
        }

	    break;
    case ETargetState::TS_Egg:

        if (ControlledEnemy->SeenPlayer)
        {
            DistanceCheck();

            Target = ClosestTarget;
            CurrentTargetState = (Target == TargetEgg) ? ETargetState::TS_Egg : ETargetState::TS_Player;
        }
        else
        {
            CurrentTargetState = ETargetState::TS_Egg;
            Target = TargetEgg;
        }

	    break;
    case ETargetState::TS_Update:

        if (TargetEgg && ControlledEnemy->SeenPlayer)
        {
            DistanceCheck();

            Target = ClosestTarget;
            CurrentTargetState = (Target == TargetEgg) ? ETargetState::TS_Egg : ETargetState::TS_Player;
        }
        else if(TargetEgg)
        {
            CurrentTargetState = ETargetState::TS_Egg;
            Target = TargetEgg;
        }
        else if(ControlledEnemy->SeenPlayer)
        {
            CurrentTargetState = ETargetState::TS_Player;
            Target = ControlledEnemy->SeenPlayer;
        }
        else
        {
            CurrentTargetState = NewTargetState;
        }

	    break;
    }

    GetBlackboardComponent()->SetValueAsObject(FName("Target"), Target);
    CurrentTargetChangedDelegate.Broadcast(CurrentTargetState);
}

AActor* AEnemyControllerBase::FindClosestActor(const AActor* First, const AActor* Second) const
{
    if (!First || !Second) return nullptr;

    const FVector Location = GetPawn()->GetActorLocation();

    const float DistanceToFirst = FVector::DistSquared(First->GetActorLocation(), Location);
    const float DistanceToSecond = FVector::DistSquared(Second->GetActorLocation(), Location);

    if (DistanceToFirst < DistanceToSecond)
    {
        return const_cast<AActor*>(First);
    }
    else
    {
        return const_cast<AActor*>(Second);
    }
}

void AEnemyControllerBase::SetBlackboardData_Implementation() { }