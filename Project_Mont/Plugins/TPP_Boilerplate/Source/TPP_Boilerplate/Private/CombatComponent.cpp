#include "CombatComponent.h"

#include "CrosshairUtility.h"
#include "TPPCharacter.h"
#include "TPPController.h"
#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MeleeWeapon.h"
#include "ProjectileWeapon.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	AimWalkSpeed = 250.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, IsAiming);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ATPPCharacter>(GetOwner());

	SetDefaults();
	Character->SetupPlayerInputDelegate.AddDynamic(this, &UCombatComponent::SetupInput);
}

void UCombatComponent::SetDefaults()
{
	CanAttack = true;

	if (Character)
	{
		BaseWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}
}

void UCombatComponent::SetupInput()
{
	if (const auto InputSubsystem = Character->GetInputSubsystem())
	{
		InputSubsystem->AddMappingContext(CombatMappingContext, 0);
	}

	if (const auto InputComponent = Character->GetInputComponent())
	{
		InputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &UCombatComponent::StartAttack);
		InputComponent->BindAction(AttackAction, ETriggerEvent::Canceled, this, &UCombatComponent::StopAttack);
		InputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &UCombatComponent::StopAttack);

		InputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &UCombatComponent::ToggleAiming);
		InputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &UCombatComponent::ToggleAiming);
		InputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &UCombatComponent::ToggleAiming);
	}
}

// TODO: Move a lot of this to the player character so other components can use it (HitResult && HUDPackage)
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		CrosshairUtility::TraceUnderCrosshairs(Character, HitResult, AimSnapOffset);
		HitTarget = HitResult.ImpactPoint;

		// Hit Target Debugging
		/*{
			DrawDebugSphere(GetWorld(), HitTarget, 12, 12, FColor::Red);

			if (GEngine)
			{
				if (HitResult.GetActor())
					GEngine->AddOnScreenDebugMessage(1, 15.0f, FColor::Blue, FString::Printf(TEXT("Hit: %s"), *HitResult.GetActor()->GetActorNameOrLabel()));
				else
					GEngine->AddOnScreenDebugMessage(1, 15.f, FColor::Red, FString::Printf(TEXT("Target Not Found")));
			}
		}*/

		if(EquippedWeapon)
		{
			if (HitResult.GetActor() && HitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
			{
				const IInteractWithCrosshairsInterface* Interface = Cast<IInteractWithCrosshairsInterface>(HitResult.GetActor());
				HUDPackage.CrosshairColor = Interface->GetColor();
			}
			else
			{
				HUDPackage.CrosshairColor = EquippedWeapon->BaseCrosshairColor;
			}
		}

		SetCombatCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::SetCombatCrosshairs(const float DeltaTime)
{
	if (!Character || !Character->Controller || !EquippedWeapon) return;

	Controller = Controller == nullptr ? Cast<ATPPController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ATPPHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			HUDPackage.CombatMode = true;
			HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
			HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
			HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
			HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
			HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;

			const FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			const FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0;

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if (IsAiming)
			{
				if (const AProjectileWeapon* ProjectileWeapon = Cast<AProjectileWeapon>(EquippedWeapon))
					CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, -.58f, DeltaTime, ProjectileWeapon->GetZoomInterpSpeed());
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0, DeltaTime, ZoomInterpSpeed);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0, DeltaTime, 20.f);

			HUDPackage.CrosshairSpread = .5f + CrosshairVelocityFactor + CrosshairInAirFactor + CrosshairAimFactor + CrosshairShootingFactor;

			HUDPackage.CrosshairSize = EquippedWeapon->CrosshairSize;

			HUD->SetHudPackage(HUDPackage);
		}
	}
}

/*
 *	Equip Weapon
 */

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(!Character || !WeaponToEquip) return;

	EquippedWeapon = WeaponToEquip;

	switch(EquippedWeapon->GetWeaponType())
	{
	case EWeaponType::EWT_TwoHandedMelee:

		if (const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("TwoHandedMelee")))
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());

		break;

	case EWeaponType::EWT_Rifle:

		if (const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("Rifle")))
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());

		break;
	default: 
		break;
	}
}

void UCombatComponent::StartAttack()
{
	IsAttackButtonPressed = true;
	if (EquippedWeapon)
	{
		Attack();
	}
}

void UCombatComponent::Attack()
{
	if (CanAttack && EquippedWeapon)
	{
		CanAttack = false;
		CrosshairShootingFactor = .5f;
		ServerAttack();

		// Attack Loop
		StartAttackTimer();
	}
}

void UCombatComponent::ServerAttack_Implementation()
{
	MulticastAttack();
}

void UCombatComponent::MulticastAttack_Implementation()
{
	if (!EquippedWeapon) return;

	if (Character)
	{
		// TODO
		// Character->PlayFireMontage(IsAiming);
		EquippedWeapon->Attack(HitTarget);
	}
}

void UCombatComponent::StartAttackTimer()
{
	if (!Character) return;
	Character->GetWorldTimerManager().SetTimer(AttackTimer, this, &UCombatComponent::AttackTimerFinished, EquippedWeapon->GetAttackDelay());
}

void UCombatComponent::AttackTimerFinished()
{
	CanAttack = true;
	if(IsAttackButtonPressed)
	{
		Attack();
	}
}

void UCombatComponent::StopAttack()
{
	IsAttackButtonPressed = false;
}

/*
 *	Aiming
 */

void UCombatComponent::ToggleAiming()
{
	if(EquippedWeapon && Cast<AMeleeWeapon>(EquippedWeapon)) return;

	// Update Client
	IsAiming = !IsAiming;
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = IsAiming ? AimWalkSpeed : BaseWalkSpeed;
		Character->SetCameraSensitivity(IsAiming ? AimSensitivity : Character->GetCurrentCameraSensitivity());
	}

	// Update Server
	ServerSetAiming(IsAiming);
}

void UCombatComponent::ServerSetAiming_Implementation(bool Aiming)
{
	IsAiming = Aiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = IsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (!EquippedWeapon) return;

	if (const AProjectileWeapon* ProjectileWeapon = Cast<AProjectileWeapon>(EquippedWeapon))
	{
		if (IsAiming)
		{
			CurrentFOV = FMath::FInterpTo(CurrentFOV, ProjectileWeapon->GetZoomedFOV(), DeltaTime, ProjectileWeapon->GetZoomInterpSpeed());
		}
		else
		{
			CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
		}

		if (Character && Character->GetFollowCamera())
		{
			Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
		}
	}
}