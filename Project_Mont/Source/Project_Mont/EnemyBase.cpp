#include "EnemyBase.h"

#include "Projectile.h"
#include "Components/CapsuleComponent.h"

AEnemyBase::AEnemyBase()
{
	EnemyHealth = Health(MaxHealth);

	GetCapsuleComponent()->SetCollisionObjectType(ECC_PhysicsBody);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	GetMesh()->SetCollisionObjectType(ECC_WorldDynamic);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		MeshComponent->OnComponentHit.AddDynamic(this, &AEnemyBase::OnHit);

		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MeshComponent->GetMaterial(0), this);
		if (DynamicMaterialInstance)
		{
			MeshComponent->SetMaterial(0, DynamicMaterialInstance);
		}
	}
}

void AEnemyBase::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (const AProjectile* Projectile = Cast<AProjectile>(OtherActor))
	{
		if ((EnemyHealth -= Projectile->Damage) <= 0) { Destroy(); }

		Damaged();
	}
}

void AEnemyBase::Damaged()
{
	if (DynamicMaterialInstance && DamageMaterial)
	{
		DynamicMaterialInstance->SetScalarParameterValue("EmissiveMultiplier", .2f);

		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
			{
				DynamicMaterialInstance->SetScalarParameterValue("EmissiveMultiplier", 0);
			}, 0.1f, false);
	}
}
