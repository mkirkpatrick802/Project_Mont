#pragma once

#include "CoreMinimal.h"
#include "Health.h"
#include "InteractWithCrosshairsInterface.h"
#include "VoxelCharacter.h"
#include "EnemyBase.generated.h"

UCLASS()
class PROJECT_MONT_API AEnemyBase : public AVoxelCharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:

	AEnemyBase();
	virtual void BeginPlay() override;


protected:

	virtual void Damaged();

private:

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:

	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHealth = 10;

private:
	Health EnemyHealth;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	UMaterialInterface* DamageMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterialInstance;

public:

	// For Crosshair Interaction
	FORCEINLINE virtual FLinearColor GetColor() const override { return FLinearColor::Red; }
};
