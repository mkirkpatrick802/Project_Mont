#pragma once

#include "CoreMinimal.h"
#include "InteractableObject.h"
#include "PickupObject.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class UAnimationAsset;
class UWidgetComponent;
class USphereComponent;

UCLASS()
class TPP_BOILERPLATE_API AWeapon : public APickupObject
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

public:	

	AWeapon();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void Fire(const FVector& HitTarget);

protected:

	UFUNCTION(BlueprintCallable)
	bool EquipRequest(const ATPPCharacter* Player);

private:

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* FireAnimation;

public:

	/*
	 *  Weapon Crosshairs Textures
	 */

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsBottom;

	/*
	*	Weapon Crosshair Settings
	*/

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	FLinearColor BaseCrosshairColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float CrosshairSize = .8;

	/*
	*	Zoomed FOV while aiming
	*/

	UPROPERTY(EditAnywhere, Category=Aiming)
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

	UPROPERTY(EditAnywhere, Category = Combat)
	TSubclassOf<class ACasing> CasingClass;

public:

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFov; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }

};
