#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyControllerBase.generated.h"

UCLASS()
class PROJECT_MONT_API AEnemyControllerBase : public AAIController
{
	GENERATED_BODY()

public:

	virtual void OnPossess(APawn* InPawn) override;

protected:

	UFUNCTION(BlueprintNativeEvent, Category=Brain)
	void SetBlackboardData();


private:

	UFUNCTION()
	void InitAIBrain();

protected:

	UPROPERTY(EditAnywhere, Category=Brain)
	UBehaviorTree* BehaviorTree;
};
