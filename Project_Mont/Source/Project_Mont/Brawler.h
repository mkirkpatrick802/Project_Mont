#pragma once

#include "CoreMinimal.h"
#include "EnemyCharacterBase.h"
#include "Brawler.generated.h"

UENUM(BlueprintType)
enum class EBrawlerEnemyState : uint8
{
    BES_Wander UMETA(DisplayName = "Wander State"),
    BES_Chase UMETA(DisplayName = "Chase State"),
    BES_Charge UMETA(DisplayName = "Charge State")
};

UCLASS()
class PROJECT_MONT_API ABrawler : public AEnemyCharacterBase
{
	GENERATED_BODY()

public:

    virtual void BeginPlay() override;

    virtual void EggStateChanged(bool IsActive) override;

    UFUNCTION(BlueprintCallable)
    void SetCurrentState(EBrawlerEnemyState NewState);

protected:

    UPROPERTY(EditAnywhere, Category = "Wander State")
    float WanderMaxWalkSpeed;

    UPROPERTY(EditAnywhere, Category = "Chase State")
    float ChaseMaxWalkSpeed;

    UPROPERTY(EditAnywhere, Category = "Charge State")
    float ChargeMaxWalkSpeed;

private:

    EBrawlerEnemyState CurrentState;
};
