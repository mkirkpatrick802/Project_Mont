#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyControllerBase.generated.h"

class AEnemyCharacterBase;
class AEgg;

UCLASS()
class PROJECT_MONT_API AEnemyControllerBase : public AAIController
{
	GENERATED_BODY()

public:

	virtual void OnPossess(APawn* InPawn) override;

	void SetEggTarget(bool ShouldTargetEgg);

protected:

	UFUNCTION(BlueprintNativeEvent, Category=Brain)
	void SetBlackboardData();

private:

	UFUNCTION()
	void InitAIBrain();

protected:

	UPROPERTY()
	UBehaviorTree* BehaviorTree;

	UPROPERTY(BlueprintReadOnly, Category = Egg)
	AEgg* TargetEgg;

	UPROPERTY(BlueprintReadOnly)
	AEnemyCharacterBase* ControlledEnemy;

private:

	UPROPERTY(EditAnywhere, Category = Egg)
	TSubclassOf<AEgg> Egg;
};
