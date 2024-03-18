#pragma once

#include "CoreMinimal.h"
#include "InteractWithCrosshairsInterface.h"
#include "GameFramework/Character.h"
#include "TurningInPlace.h"
#include "TPPCharacter.generated.h"

class AInteractableObject;
class AWeapon;
struct FInputActionValue;

UCLASS()
class TPP_BOILERPLATE_API ATPPCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{

	GENERATED_BODY()

/*
 *		Components
 */

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = Combat)
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere, Category = Interaction)
	class UInteractComponent* InteractComponent;

	UPROPERTY(VisibleAnywhere, Category = Inventory)
	class UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, Category = Interaction)
	class USphereComponent* InteractionSphere;

/*
 *		Inputs
 */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DropAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;


public:

	ATPPCharacter();

	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnRep_ReplicatedMovement() override;

	bool IsWeaponEquipped();
	bool IsHoldingObject();

	AWeapon* GetEquippedWeapon();
	AInteractableObject* GetHeldObject();
	bool IsAiming();

	void PlayFireMontage(bool IsAiming);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHit();

	// For Crosshair Interaction
	virtual FLinearColor GetColor() const override;

protected:

	virtual void BeginPlay() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleCrouch(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void ToggleAim(const FInputActionValue& Value);
	void DropObject(const FInputActionValue& Value);

	void CalculateAO_Pitch();
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();

	virtual void Jump() override;

	void Attack(const FInputActionValue& Value);
	void StopFiringWeapon(const FInputActionValue& Value);

	void PlayHitReactMontage();

private:

	void TurnInPlace(float DeltaTime);
	void HideCameraIfCharacterClose();
	float CalculateSpeed();

private:

	// Animation
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Camera)
	float CameraThreshold = 200.f;

	bool RotateRootBone;
	float TurnThreshold = .5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

public:

	/**
	*	Getters & Setters
	*/

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return RotateRootBone; }
	FVector GetHitTarget() const;
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE USphereComponent* GetInteractionSphere() const { return InteractionSphere; }

	UFUNCTION(BlueprintCallable)
	FORCEINLINE UInteractComponent* GetInteractComponent() const { return InteractComponent; }

	FORCEINLINE UCombatComponent* GetCombatComponent() const { return Combat; }
};