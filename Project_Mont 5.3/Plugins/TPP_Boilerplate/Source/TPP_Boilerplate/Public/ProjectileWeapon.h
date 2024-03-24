#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

UCLASS()
class TPP_BOILERPLATE_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:

	virtual void Attack(const FVector& HitTarget) override;

protected:

	/*
	*	Zoomed FOV while aiming
	*/

	UPROPERTY(EditAnywhere, Category = Aiming)
	float ZoomedFov = 40;

	UPROPERTY(EditAnywhere, Category = Aiming)
	float ZoomInterpSpeed = 20;

private:

	UPROPERTY(EditAnywhere, Category=Projectile)
	TSubclassOf<class AProjectile> ProjectileClass;

public:

	FORCEINLINE float GetZoomedFOV() const { return ZoomedFov; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }

};
