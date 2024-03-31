#pragma once

#include "CoreMinimal.h"
#include "DamageableInterface.h"
#include "Health.h"
#include "InteractWithCrosshairsInterface.h"
#include "GameFramework/Character.h"
#include "TurningInPlace.h"
#include "TPPCharacter.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class ATPPController;
class AInteractableObject;
class AWeapon;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSetupPlayerInputDelegate);

UCLASS()
class TPP_BOILERPLATE_API ATPPCharacter : public ACharacter, public IInteractWithCrosshairsInterface, public IDamageableInterface
{

	GENERATED_BODY()

	/*
	*	Components
	*/

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = Combat)
	class UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, Category = Interaction)
	class UInteractComponent* InteractComponent;

	UPROPERTY(VisibleAnywhere, Category = Inventory)
	class UInventoryComponent* InventoryComponent;

	/*
	*	Inputs
	*/

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Input", meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* MovementMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Input", meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

public:

	ATPPCharacter();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnRep_ReplicatedMovement() override;

	// For Crosshair Interaction
	virtual FLinearColor GetColor() const override;

	/*
	*	Animations
	*/

	void PlayAttackMontage(UAnimMontage* AttackMontage);
	void PlayHitReactMontage();

protected:

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void ToggleCrouch(const FInputActionValue& Value);

	void CalculateAO_Pitch();
	void AimOffset(const float DeltaTime);
	void SimProxiesTurn();

	virtual void Jump() override;


private:

	virtual void Hit(const float Damage) override;

	void TurnInPlace(float DeltaTime);
	void HideCharacterIfCameraClose();
	float CalculateSpeed();

public:

	FSetupPlayerInputDelegate SetupPlayerInputDelegate;

private:

	// Health
	UPROPERTY(EditAnywhere, Category=Health)
	int MaxHealth = 150;

	Health PlayerHealth;

	// References
	UPROPERTY()
	ATPPController* PlayerController;

	// Input
	UPROPERTY()
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem;

	UPROPERTY()
	UEnhancedInputComponent* EnhancedInputComponent;

	float CurrentDeltaTime;

	// Camera Settings
	UPROPERTY(EditAnywhere, Category = Camera)
	float CameraThreshold = 200.f;

	UPROPERTY(EditAnywhere, Category=Camera)
	float BaseSensitivity;

	float CurrentCameraSensitivity = 1;

	// Animation
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;

	bool RotateRootBone;
	float TurnThreshold = .5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

public:

	/*
	*	Getters & Setters
	*/

	/*
	*	Input
	*/

	FORCEINLINE UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem() const { return InputSubsystem; }
	FORCEINLINE UEnhancedInputComponent* GetInputComponent() const { return EnhancedInputComponent; }

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return RotateRootBone; }
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	UFUNCTION(BlueprintCallable)
	FORCEINLINE UInteractComponent* GetInteractComponent() const { return InteractComponent; }

	FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComponent; }

	FORCEINLINE float GetCurrentCameraSensitivity() const { return CurrentCameraSensitivity; }
	FORCEINLINE void SetCameraSensitivity(const float NewSensitivity) { CurrentCameraSensitivity = NewSensitivity; }
};