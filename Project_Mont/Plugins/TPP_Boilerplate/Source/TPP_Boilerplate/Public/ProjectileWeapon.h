#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

UCLASS()
class TPP_BOILERPLATE_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:

	virtual void Fire(const FVector& HitTarget) override;

protected:

	/*
	*	Zoomed FOV while aiming
	*/

	UPROPERTY(EditAnywhere, Category = Aiming)
	float ZoomedFov = 40;

	UPROPERTY(EditAnywhere, Category = Aiming)
	float ZoomInterpSpeed = 20;

	/*
	*	Automatic Fire
	*/

	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = .15f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool IsAutomatic = true;

private:

	UPROPERTY(EditAnywhere, Category=Projectile)
	TSubclassOf<class AProjectile> ProjectileClass;

public:

	FORCEINLINE float GetZoomedFOV() const { return ZoomedFov; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE bool GetAutomaticState() const { return IsAutomatic; }

};
