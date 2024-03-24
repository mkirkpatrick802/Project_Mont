#pragma once

#include "CoreMinimal.h"
#include "Health.h"
#include "InteractWithCrosshairsInterface.h"
#include "GameFramework/Character.h"
#include "EnemyCharacterBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAttackFinishedDelegate);

UCLASS()
class PROJECT_MONT_API AEnemyCharacterBase : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:

	AEnemyCharacterBase();
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category=Combat)
	virtual void MeleeAttack();

protected:

	virtual void Damaged();

private:

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* AnimMontage, bool bArg);

public:

	UPROPERTY(BlueprintAssignable)
	FAttackFinishedDelegate AttackFinishedDelegate;

protected:

	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHealth = 10;

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* AttackMontage;

private:

	Health HitPoints;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterialInstance;

	UPROPERTY()
	UAnimInstance* AnimationInstance;

public:

	// For Crosshair Interaction
	FORCEINLINE virtual FLinearColor GetColor() const override { return FLinearColor::Red; }

};
