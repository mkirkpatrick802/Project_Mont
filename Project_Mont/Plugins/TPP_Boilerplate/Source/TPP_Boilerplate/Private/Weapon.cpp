#include "Weapon.h"

#include "Casing.h"
#include "CombatComponent.h"
#include "TPPCharacter.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ModelComponent->UnregisterComponent();
	ModelComponent->DestroyComponent(false);

	ModelComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
	ModelComponent->SetupAttachment(RootComponent);
	SetRootComponent(ModelComponent);

	InteractWidget->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	WeaponMesh = Cast<USkeletalMeshComponent>(ModelComponent);
	WeaponMesh->SetGenerateOverlapEvents(true);

	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/*
 *		Fire
 */

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}

	if (CasingClass)
	{
		if (const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject")))
		{
			const FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			if (UWorld* World = GetWorld())
			{
				const FActorSpawnParameters SpawnParams;
				World->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), SpawnParams);
			}
		}
	}
}

void AWeapon::Melee()
{

}

bool AWeapon::EquipRequest(const ATPPCharacter* Player)
{
	if(UCombatComponent* Combat = Player->GetCombatComponent())
	{
		Combat->EquipWeapon(this);
		return true;
	}

	return false;
}
