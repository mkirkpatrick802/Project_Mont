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
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
			FActorSpawnParameters SpawnParams;
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), SpawnParams);
			}
		}
	}
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