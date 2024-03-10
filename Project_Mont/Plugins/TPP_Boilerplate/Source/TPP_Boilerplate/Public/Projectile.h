#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class TPP_BOILERPLATE_API AProjectile : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Projectile)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere, Category = Projectile)
	class UProjectileMovementComponent* ProjectileMovementComponent;
	
public:	

	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:

private:

	UPROPERTY(EditAnywhere, Category = Projectile)
	class UParticleSystem* Tracer;

	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere, Category = Projectile)
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, Category = Projectile)
	class USoundCue* ImpactSound;

};
