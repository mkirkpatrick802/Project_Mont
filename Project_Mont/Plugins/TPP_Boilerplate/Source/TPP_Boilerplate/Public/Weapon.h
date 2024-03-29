#pragma once

#include "CoreMinimal.h"
#include "PickupObject.h"
#include "Weapon.generated.h"

class UAnimationAsset;
class UWidgetComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_TwoHandedMelee UMETA(DisplayName = "Two Handed Melee"),
	EWT_Rifle UMETA(DisplayName = "Rifle"),

	EWT_Max UMETA(DisplayName = "Max"),
};

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

	virtual void Attack(const FVector& HitTarget = FVector::ZeroVector);

private:

	UPROPERTY(EditAnywhere, Category = "Weapon Animation")
	UAnimationAsset* AttackAnimation;

public:

	/*
	*	Weapon Damage
	*/

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float AttackDelay = .15f;

	/*
	 *	Weapon Damage
	 */

	UPROPERTY(EditAnywhere, Category = "Weapon Stats")
	float Damage = 2;

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

protected:

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EWeaponType WeaponType = EWeaponType::EWT_Rifle;

public:

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE float GetAttackDelay() const { return AttackDelay; }

};
