#include "TPPCharacter.h"

#include "CombatComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InteractComponent.h"
#include "InventoryComponent.h"
#include "TPPController.h"
#include "Types.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

ATPPCharacter::ATPPCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->RotationRate = FRotator(0, 600, 0);
	GetCharacterMovement()->MaxWalkSpeed = 600;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 400;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->SetRelativeLocation(FVector(0, 0, 100));
	CameraBoom->SocketOffset = FVector(0, 75, 75);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Follow Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));
	CombatComponent->SetIsReplicated(true);

	InteractComponent = CreateDefaultSubobject<UInteractComponent>(TEXT("Interaction Component"));

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory Component"));

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	PlayerHealth = MaxHealth;
}

void ATPPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ATPPCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentCameraSensitivity = BaseSensitivity;
	PlayerController = Cast<ATPPController>(Controller);
	check(PlayerController != nullptr)

	//Add Input Mapping Context
	InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

	if(InputSubsystem)
		InputSubsystem->AddMappingContext(MovementMappingContext, 0);

	SetupPlayerInputDelegate.Broadcast();
}

void ATPPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings

	EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	check(EnhancedInputComponent != nullptr)

	//Jumping
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ATPPCharacter::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ACharacter::StopJumping);
	
	//Moving
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATPPCharacter::Move);
	
	//Looking
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATPPCharacter::Look);
	
	//Crouching
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ATPPCharacter::ToggleCrouch);
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ATPPCharacter::ToggleCrouch);
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Canceled, this, &ATPPCharacter::ToggleCrouch);
}

void ATPPCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CurrentDeltaTime = DeltaTime;

	if(GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if(TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}

		CalculateAO_Pitch();
	}

	HideCharacterIfCameraClose();
}

void ATPPCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ATPPCharacter::HideCharacterIfCameraClose()
{
	if(!InteractComponent) return;

	if (!IsLocallyControlled()) return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		InteractComponent->ToggleHeldObjectVisibility(false);
	}
	else
	{
		GetMesh()->SetVisibility(true);
		InteractComponent->ToggleHeldObjectVisibility(true);
	}
}

/*
 *	Character Movement
 */

void ATPPCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ATPPCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const float HorizontalAxis = LookAxisVector.X * CurrentCameraSensitivity;
		const float VerticalAxis = LookAxisVector.Y * CurrentCameraSensitivity;

		AddControllerYawInput(HorizontalAxis);
		AddControllerPitchInput(VerticalAxis);
	}
}

void ATPPCharacter::ToggleCrouch(const FInputActionValue& Value)
{
	if(GetCharacterMovement()->IsFalling()) return;

	if(bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ATPPCharacter::Jump()
{
	if (bIsCrouched) return;

	Super::Jump();
}

/*
 *  Animations
 */

void ATPPCharacter::PlayAttackMontage(UAnimMontage* AttackMontage)
{

}

void ATPPCharacter::PlayHitReactMontage()
{

}

/*
 *	Aim Offset
 */

void ATPPCharacter::AimOffset(float DeltaTime)
{
	if (!CombatComponent || !CombatComponent->IsWeaponEquipped()) return;

	const float Speed = CalculateSpeed();
	const bool IsInAir = GetCharacterMovement()->IsFalling();

	if(Speed == 0 && !IsInAir)
	{
		RotateRootBone = true;
		const FRotator CurrentAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;

		if(TurningInPlace == ETurningInPlace::ETIP_NotTurning)
			InterpAO_Yaw = AO_Yaw;

		bUseControllerRotationYaw = true;
		CameraBoom->bEnableCameraRotationLag = false;
		TurnInPlace(DeltaTime);
	}

	if(Speed > 0 || IsInAir)
	{
		RotateRootBone = false;
		StartingAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		AO_Yaw = 0;

		bUseControllerRotationYaw = true;
		CameraBoom->bEnableCameraRotationLag = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

/*
 *	Turn In-place
 */

void ATPPCharacter::SimProxiesTurn()
{
	RotateRootBone = false;

	const float Speed = CalculateSpeed();
	if(Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if(FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if(ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if(ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}

		return;
	}

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ATPPCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// Map pitch from [270, 360) to [-90, 0)
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ATPPCharacter::TurnInPlace(float DeltaTime)
{
	if(AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if(AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if(TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0, DeltaTime, 4.f);

		AO_Yaw = InterpAO_Yaw;
		if(FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		}
	}
}

float ATPPCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;
	return Velocity.Length();
}

/*
 *	Interfaces
 */

void ATPPCharacter::Hit(const float Damage)
{
	PlayerHealth -= Damage;
	if (PlayerHealth.GetCurrentHealth() <= 0)
		Destroy();
}

FLinearColor ATPPCharacter::GetColor() const
{
	return FLinearColor::Red;
}