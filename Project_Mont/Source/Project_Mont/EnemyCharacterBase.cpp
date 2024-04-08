#include "EnemyCharacterBase.h"

#include "DamageableInterface.h"
#include "EnemyControllerBase.h"
#include "Projectile.h"
#include "Project_Mont.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyCharacterBase::AEnemyCharacterBase()
{
	GetCapsuleComponent()->SetCollisionObjectType(ECC_PhysicsBody);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	GetMesh()->SetCollisionObjectType(ECC_WorldDynamic);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
}

void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	HitPoints = Health(MaxHealth);

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		MeshComponent->OnComponentHit.AddDynamic(this, &AEnemyCharacterBase::OnHit);

		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MeshComponent->GetMaterial(1), this);
		if (DynamicMaterialInstance)
		{
			MeshComponent->SetMaterial(1, DynamicMaterialInstance);
		}

		AnimationInstance = MeshComponent->GetAnimInstance();
	}

	const float DelayTime = 0.2f;
	FTimerHandle DelayTimerHandle;
	GetWorldTimerManager().SetTimer(DelayTimerHandle, this, &AEnemyCharacterBase::DelayedStart, DelayTime, false);
}

void AEnemyCharacterBase::DelayedStart()
{
	EnemyController = Cast<AEnemyControllerBase>(Controller);
}

void AEnemyCharacterBase::MeleeAttack()
{
	if(AttackMontage && AnimationInstance)
	{
		AnimationInstance->Montage_Play(AttackMontage, 1);

		FOnMontageEnded MontageEndedDelegate;
		MontageEndedDelegate.BindUObject(this, &AEnemyCharacterBase::OnMontageEnded);
		AnimationInstance->Montage_SetEndDelegate(MontageEndedDelegate);
	}
}

void AEnemyCharacterBase::MeleeAttackCheck()
{
	if (!GetMesh() || AttackHit) return;

	FVector StartLocation = GetMesh()->GetSocketLocation(TEXT("RightHandSocket"));
	FVector ForwardVector = GetActorForwardVector();
	FVector EndLocation = StartLocation + (ForwardVector * 40);

	FHitResult OutHit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(OutHit, StartLocation, EndLocation, ECC_HitBox, CollisionParams) && OutHit.GetActor())
	{
		if (OutHit.GetActor()->Implements<UDamageableInterface>())
		{
			IDamageableInterface* HitObject = Cast<IDamageableInterface>(OutHit.GetActor());
			HitObject->Hit(Damage);

			AttackHit = true;
		}

		DrawDebugLine(GetWorld(), StartLocation, OutHit.ImpactPoint, FColor::Red, false, 1.0f, 0, 1.0f);
		DrawDebugLine(GetWorld(), OutHit.ImpactPoint, EndLocation, FColor::Green, false, 1.0f, 0, 1.0f);
	}
}

void AEnemyCharacterBase::MeleeAttackEnd()
{
	AttackHit = false;
}

void AEnemyCharacterBase::EggStateChanged(const bool IsActive)
{
	if(EnemyController)
		EnemyController->SetEggTarget(IsActive);
}

void AEnemyCharacterBase::OnMontageEnded(UAnimMontage* AnimMontage, bool bArg)
{
	if (AnimMontage == AttackMontage)
		AttackFinishedDelegate.Broadcast();
}

void AEnemyCharacterBase::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (const AProjectile* Projectile = Cast<AProjectile>(OtherActor))
	{
		if ((HitPoints -= Projectile->Damage) <= 0)
		{
			HasDiedDelegate.Broadcast(this);
			return;
		}

		Damaged();
	}
}

void AEnemyCharacterBase::Damaged()
{
	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		MeshComponent->SetMaterial(1, HitMaterial);

		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
			{
				GetMesh()->SetMaterial(1, DynamicMaterialInstance);
			}, 0.1f, false);
	}
}
