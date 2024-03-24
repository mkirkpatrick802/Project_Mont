#include "BuildingPieceBase.h"

ABuildingPieceBase::ABuildingPieceBase()
{
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh"));
	StaticMesh->SetupAttachment(RootComponent);
	SetRootComponent(StaticMesh);
}

void ABuildingPieceBase::BeginPlay()
{
	Super::BeginPlay();

	DefaultMaterial = StaticMesh->GetMaterial(0);
	StaticMesh->SetMaterial(0, PreviewMaterial);

	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABuildingPieceBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABuildingPieceBase::Placed() const
{
	StaticMesh->SetMaterial(0, DefaultMaterial);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}