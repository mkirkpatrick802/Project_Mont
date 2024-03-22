#include "Weapon.h"

#include "Components/WidgetComponent.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent->UnregisterComponent();
	MeshComponent->DestroyComponent(false);

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
	MeshComponent->SetupAttachment(RootComponent);
	SetRootComponent(MeshComponent);

	InteractWidget->SetupAttachment(RootComponent);

	WeaponMesh = Cast<USkeletalMeshComponent>(MeshComponent);
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

void AWeapon::Attack(const FVector& HitTarget)
{
	if (AttackAnimation)
	{
		WeaponMesh->PlayAnimation(AttackAnimation, false);
	}
}