#pragma once

#include "CoreMinimal.h"
#include "DamageableInterface.h"
#include "Health.h"
#include "PickupObject.h"
#include "Egg.generated.h"

UCLASS()
class PROJECT_MONT_API AEgg : public APickupObject, public IDamageableInterface
{
	GENERATED_BODY()

public:

	AEgg();
	virtual void BeginPlay() override;
	virtual void Hit(float Damage) override;

protected:

	UPROPERTY(EditAnywhere, Category=Health)
	float MaxHealth = 100;

private:

	Health EggHealth;
};
