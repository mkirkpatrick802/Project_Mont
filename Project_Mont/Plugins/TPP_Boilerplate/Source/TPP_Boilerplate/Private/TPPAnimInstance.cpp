#include "TPPAnimInstance.h"

#include "TPPCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon.h"

void UTPPAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	TPPCharacter = Cast<ATPPCharacter>(TryGetPawnOwner());
}

void UTPPAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if(!TPPCharacter)
	{
		TPPCharacter = Cast<ATPPCharacter>(TryGetPawnOwner());
	}

	if(!TPPCharacter) return;

	FVector Velocity = TPPCharacter->GetVelocity();
	Velocity.Z = 0;
	Speed = Velocity.Length();

	IsInAir = TPPCharacter->GetCharacterMovement()->IsFalling();
	IsAccelerating = TPPCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;
	IsCrouched = TPPCharacter->bIsCrouched;
	TurningInPlace = TPPCharacter->GetTurningInPlace();

	IsHoldingObject = TPPCharacter->IsHoldingObject();
	HeldObject = TPPCharacter->GetHeldObject();

	IsWeaponEquipped = TPPCharacter->IsWeaponEquipped();
	EquippedWeapon = TPPCharacter->GetEquippedWeapon();
	IsAiming = TPPCharacter->IsAiming();
	RotateRootBone = TPPCharacter->ShouldRotateRootBone();

	// Offset Yaw for Strafing
	const FRotator AimRotation = TPPCharacter->GetBaseAimRotation();
	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(TPPCharacter->GetVelocity());
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 3.f);
	YawOffset = DeltaRotation.Yaw;

	// Leaning
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = TPPCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = TPPCharacter->GetAO_Yaw();
	AO_Pitch = TPPCharacter->GetAO_Pitch();

	if (IsWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && TPPCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		TPPCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (TPPCharacter->IsLocallyControlled())
		{
			IsLocallyControlled = true;
			const FTransform RightHandTransform = TPPCharacter->GetMesh()->GetSocketTransform(FName("hand_r"), RTS_World);
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - TPPCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds, 40.f);
		}
	}
}