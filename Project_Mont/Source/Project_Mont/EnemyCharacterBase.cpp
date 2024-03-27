#include "EnemyCharacterBase.h"

#include "EnemyControllerBase.h"
#include "Projectile.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyCharacterBase::AEnemyCharacterBase()
{
	HitPoints = Health(MaxHealth);

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
	if (DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue("EmissiveMultiplier", .2f);

		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
			{
				DynamicMaterialInstance->SetScalarParameterValue("EmissiveMultiplier", 0);
			}, 0.1f, false);
	}
}
