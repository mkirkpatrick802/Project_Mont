#pragma once

#include "CoreMinimal.h"
#include "TPPHUD.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class ATPPCharacter;
class UInputAction;
class UInputMappingContext;
class AWeapon;
class ATPPHUD;
class ATPPController;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPP_BOILERPLATE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

	/*
	*	Inputs
	*/

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* CombatMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;


public:	

	UCombatComponent();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void EquipWeapon(AWeapon* WeaponToEquip);

private:

	/*
	*	Init
	*/

	UFUNCTION()
	void SetupInput();

	void SetDefaults();

	/*
	*	Standard Attack
	*/

	void StartAttack();
	void Attack();

	UFUNCTION(Server, Reliable)
	void ServerAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAttack();

	void StartAttackTimer();
	void AttackTimerFinished();

	void StopAttack();

	/*
	*	Crosshair Setup
	*/

	void SetCombatCrosshairs(float DeltaTime);

	/*
	*	Aiming
	*/

	void ToggleAiming();

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool Aiming);

	void InterpFOV(float DeltaTime);

private:

	/*
	*	Attacking
	*/

	bool IsAttackButtonPressed;
	bool CanAttack;

	FTimerHandle AttackTimer;

	UPROPERTY()
	ATPPCharacter* Character;

	UPROPERTY()
	ATPPController* Controller;

	UPROPERTY()
	ATPPHUD* HUD;

	UPROPERTY(Replicated)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool IsAiming;

	UPROPERTY(EditAnywhere, Category = Camera)
	float AimSensitivity;

	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere, Category = Combat)
	float AimWalkSpeed;

	FHUDPackage HUDPackage;
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	/*
	 *	Aiming and FOV
	*/

	UPROPERTY(EditAnywhere, Category = Combat)
	float AimSnapOffset = 200;

	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	float CurrentFOV;

	/*
	 *	Firing
	*/

	FVector HitTarget;

public:

	FORCEINLINE bool IsWeaponEquipped() { return EquippedWeapon != nullptr; }

	FORCEINLINE void DropWeapon() { EquippedWeapon = nullptr; }

};
