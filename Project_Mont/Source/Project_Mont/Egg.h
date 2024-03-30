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
	virtual void Hit(float Damage) override;

private:

	Health EggHealth;

};
