#pragma once

#include "CoreMinimal.h"
#include "EnemyCharacterBase.h"
#include "Components/ActorComponent.h"
#include "EnemySpawnerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_MONT_API UEnemySpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UEnemySpawnerComponent();

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void SpawnWave(int EnemyCount);

private:

	void SpawnEnemy(const FVector& Location, TSubclassOf<AEnemyCharacterBase> ToSpawn) const;

	FVector FindSpawnLocation();

protected:

	UPROPERTY(EditAnywhere, Category="Enemy Spawning")
	bool Debugging;

	UPROPERTY(EditAnywhere, Category = "Enemy Spawning")
	float MinRadius;

	UPROPERTY(EditAnywhere, Category = "Enemy Spawning")
	float MaxRadius;

	UPROPERTY(EditAnywhere, Category = "Enemy Spawning")
	TSubclassOf<AEnemyCharacterBase> ChaserEnemy;

private:

	float IslandZ;

};
