#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class TPP_BOILERPLATE_API ACasing : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = Shell)
	UStaticMeshComponent* CasingMesh;
	
public:

	ACasing();
	virtual void Destroyed() override;

protected:

	virtual void BeginPlay() override;

private:

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:

	UPROPERTY(EditAnywhere, Category=Shell)
	float ShellEjectionImpulse;

	UPROPERTY(EditAnywhere, Category = Shell)
	class USoundCue* ShellSound;

};
