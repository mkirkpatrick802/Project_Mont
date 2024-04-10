#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyControllerBase.generated.h"

class ABuildingPieceBase;

UENUM(BlueprintType)
enum class ETargetState : uint8
{
	TS_Player UMETA(DisplayName = "Target Player"),
	TS_Egg UMETA(DisplayName = "Target Egg"),
	TS_Building UMETA(DisplayName = "Target Building"),
	TS_Update UMETA(DisplayName = "Update Target")
};

class AEnemyCharacterBase;
class AEgg;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCurrentTargetChanged, ETargetState, NewState);

UCLASS()
class PROJECT_MONT_API AEnemyControllerBase : public AAIController
{
	GENERATED_BODY()

public:

	AEnemyControllerBase();
	virtual void OnPossess(APawn* InPawn) override;
	void SetEggTarget(bool ShouldTargetEgg);

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category=State)
	void SetCurrentTargetState(ETargetState NewTargetState);

protected:

	UFUNCTION(BlueprintNativeEvent, Category=Brain)
	void SetBlackboardData();

private:

	UFUNCTION()
	void InitAIBrain();

	void DistanceCheck();
	AActor* FindClosestActor(const AActor* First, const AActor* Second) const;

public:

	FCurrentTargetChanged CurrentTargetChangedDelegate;

	UPROPERTY(BlueprintReadWrite)
	ABuildingPieceBase* TargetBuilding;

protected:

	UPROPERTY(EditAnywhere, Category = AI)
	UBehaviorTree* BehaviorTree;

	UPROPERTY(BlueprintReadOnly, Category = Egg)
	AEgg* TargetEgg;

private:

	UPROPERTY(EditAnywhere, Category = Egg)
	TSubclassOf<AEgg> Egg;

	ETargetState CurrentTargetState;

	UPROPERTY()
	AEnemyCharacterBase* ControlledEnemy;

	/*
	*	Closest Target
	*/

	FTimerHandle DistanceCheckHandle;

	UPROPERTY()
	AActor* ClosestTarget;

public:

	UFUNCTION(BlueprintCallable, Category=State)
	FORCEINLINE void UpdateTargetState() { SetCurrentTargetState(ETargetState::TS_Update); }

};
