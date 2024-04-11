#include "Egg.h"

#include "Components/WidgetComponent.h"

AEgg::AEgg()
{
	MeshComponent->UnregisterComponent();
	MeshComponent->DestroyComponent(false);

	HitboxCapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(FName("Hitbox Collider"));
	HitboxCapsuleComponent->SetupAttachment(RootComponent);
	SetRootComponent(HitboxCapsuleComponent);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(FName("Egg Mesh"));
	MeshComponent->SetupAttachment(RootComponent);

	StaticMesh = Cast<UStaticMeshComponent>(MeshComponent);

	InteractWidget->UnregisterComponent();
	InteractWidget->SetupAttachment(RootComponent);
}

void AEgg::BeginPlay()
{
	Super::BeginPlay();

	EggHealth = MaxHealth;
}

void AEgg::Hit(const float Damage)
{
	EggHealth -= Damage;

	if(EggHealth.GetCurrentHealth() <= 0)
	{
		Destroy();
	}
}

void AEgg::PickUp(ATPPCharacter* Player)
{
	Super::PickUp(Player);

	const auto RelativePosition = RootComponent->GetRelativeLocation();
	SetActorRelativeLocation(FVector(RelativePosition.X, RelativePosition.Y, RelativePosition.Z + 50));
}
