#include "CombatComponent.h"

#include "CrosshairUtility.h"
#include "TPPCharacter.h"
#include "TPPController.h"
#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon.h"
#include "Engine.h"
#include "Engine/World.h"

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

	if(Character)
	{
		BaseWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;

		if(Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		CrosshairUtility::TraceUnderCrosshairs(Character, HitResult, HUDPackage, true, AimSnapOffset);
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
				IInteractWithCrosshairsInterface* Interface = Cast<IInteractWithCrosshairsInterface>(HitResult.GetActor());
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

/*
 *	Equip Weapon
 */

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(!Character || !WeaponToEquip) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetObjectState(EObjectState::EWS_PickedUp);
	EquippedWeapon->TogglePhysics(false);

	if(const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")))
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());

	EquippedWeapon->SetOwner(Character);

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if(EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

/*
 *	Drop Weapon
 */

void UCombatComponent::DropWeapon()
{
	if (!Character || !EquippedWeapon) return;
	EquippedWeapon->SetObjectState(EObjectState::EWS_Dropped);
	EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	EquippedWeapon->SetOwner(nullptr);
	EquippedWeapon->TogglePhysics(true);
	EquippedWeapon = nullptr;

	Character->GetCharacterMovement()->bOrientRotationToMovement = true;
	Character->bUseControllerRotationYaw = false;
}

/*
 *	Fire Weapon
 */

void UCombatComponent::FireButtonPressed(const bool Pressed)
{
	IsFireButtonPressed = Pressed; 
	if(IsFireButtonPressed && EquippedWeapon)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if(CanFire)
	{
		CanFire = false;
		ServerFire(HitTarget);
		CrosshairShootingFactor = 0.5f;
		StartFireTimer();
	}
}

void UCombatComponent::StartFireTimer()
{
	if (!Character) return;
	Character->GetWorldTimerManager().SetTimer(FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimerFinished()
{
	CanFire = true;
	if (IsFireButtonPressed && EquippedWeapon->IsAutomatic)
	{
		Fire();
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (!EquippedWeapon) return;
	if (Character)
	{
		Character->PlayFireMontage(IsAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

/*
 *	Aiming
 */

void UCombatComponent::SetCombatCrosshairs(float DeltaTime)
{
	if(!Character || !Character->Controller || !EquippedWeapon) return;

	Controller = Controller == nullptr ? Cast<ATPPController>(Character->Controller) : Controller;
	if(Controller)
	{
		HUD = HUD == nullptr ? Cast<ATPPHUD>(Controller->GetHUD()) : HUD;
		if(HUD)
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

			if(Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if(IsAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, -.58f, DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
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

void UCombatComponent::ToggleAiming()
{
	// Update Client
	IsAiming = !IsAiming;
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = IsAiming ? AimWalkSpeed : BaseWalkSpeed;
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

	if(IsAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if(Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}