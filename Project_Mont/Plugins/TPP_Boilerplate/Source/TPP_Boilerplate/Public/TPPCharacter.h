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

	/** Components */
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

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Crouch Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Interact Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	/** Drop Object Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DropAction;

	/** Aim Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FireAction;


public:

	/**
	 *	Public Function
	 */

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

protected:

	virtual void BeginPlay() override;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for crouch input */
	void ToggleCrouch(const FInputActionValue& Value);

	/** Called for interact input */
	void Interact(const FInputActionValue& Value);

	/** Called for aim input */
	void ToggleAim(const FInputActionValue& Value);

	/** Called for drop input */
	void DropObject(const FInputActionValue& Value);

	void CalculateAO_Pitch();
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();

	virtual void Jump() override;

	void StartFiringWeapon(const FInputActionValue& Value);
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
};