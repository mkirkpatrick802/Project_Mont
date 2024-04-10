#pragma once

#include "CoreMinimal.h"
#include "Health.h"
#include "InteractWithCrosshairsInterface.h"
#include "GameFramework/Character.h"
#include "EnemyCharacterBase.generated.h"

class UPawnSensingComponent;
class UEnemySpawnerComponent;
class AEnemyControllerBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAttackFinishedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHasDiedDelegate, AEnemyCharacterBase*, DeadEnemy);

UCLASS()
class PROJECT_MONT_API AEnemyCharacterBase : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Sight)
	UPawnSensingComponent* PawnSensingComponent;

public:

	AEnemyCharacterBase();
	virtual void BeginPlay() override;
	virtual void DelayedStart();

	UFUNCTION(BlueprintCallable, Category=Combat)
	virtual void MeleeAttack();

	UFUNCTION(BlueprintCallable, Category=Combat)
	void StopAttackMontage();

	void MeleeAttackCheck();
	void MeleeAttackEnd();

	UFUNCTION()
	virtual void EggStateChanged(bool IsActive);

protected:

	virtual void Damaged();

private:

	/*
	 *	Aggro Logic
	 */

	UFUNCTION()
	void OnSeePawn(APawn* SeenPawn);

	void StartDeaggroCountdown();
	void Deaggro();

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* AnimMontage, bool bArg);

public:

	UPROPERTY(BlueprintAssignable)
	FAttackFinishedDelegate AttackFinishedDelegate;

	FHasDiedDelegate HasDiedDelegate;

	UPROPERTY()
	ACharacter* SeenPlayer;

	UPROPERTY()
	UEnemySpawnerComponent* Spawner;

protected:

	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHealth = 10;

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitMontage;

	UPROPERTY(BlueprintReadOnly)
	AEnemyControllerBase* EnemyController;

	bool IsEggActive = false;

private:

	UPROPERTY(EditAnywhere, Category=Combat)
	int Damage = 10;

	Health HitPoints;

	UPROPERTY(EditAnywhere, Category = "Hit Response")
	UMaterialInstance* HitMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterialInstance;

	UPROPERTY()
	UAnimInstance* AnimationInstance;

	bool AttackHit = false;

	/*
	 *	Aggro Logic
	 */

	FTimerHandle DeaggroTimerHandle;

	UPROPERTY(EditAnywhere, Category = Sight)
	float DeaggroTime = 1;

public:

	// For Crosshair Interaction
	FORCEINLINE virtual FLinearColor GetColor() const override { return FLinearColor::Red; }

	FORCEINLINE void SetEnemyController(AEnemyControllerBase* ControllerToSet) { EnemyController = ControllerToSet; }
};
