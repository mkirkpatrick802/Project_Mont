#include "Casing.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/StaticMeshComponent.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;
	ShellEjectionImpulse = 5;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Casing Mesh"));
	CasingMesh->SetupAttachment(RootComponent);
	SetRootComponent(CasingMesh);

	CasingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
	CasingMesh->SetUseCCD(true);
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	Destroy();
}

void ACasing::Destroyed()
{
	Super::Destroyed();
}
