#pragma once

#include "CoreMinimal.h"
#include "DamageableInterface.h"
#include "Health.h"
#include "PickupObject.h"
#include "Components/CapsuleComponent.h"
#include "Egg.generated.h"

UCLASS()
class PROJECT_MONT_API AEgg : public APickupObject, public IDamageableInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = Collider)
	UCapsuleComponent* HitboxCapsuleComponent;

public:

	AEgg();
	virtual void BeginPlay() override;
	virtual void Hit(float Damage) override;

protected:

	virtual void PickUp(ATPPCharacter* Player) override;

protected:

	UPROPERTY(EditAnywhere, Category=Health)
	float MaxHealth = 100;

private:

	Health EggHealth;
};
