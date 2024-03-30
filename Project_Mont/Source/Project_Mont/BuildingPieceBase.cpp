#include "BuildingPieceBase.h"

#include "Project_Mont.h"
#include "SocketComponent.h"

ABuildingPieceBase::ABuildingPieceBase()
{
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh"));
	StaticMesh->SetupAttachment(RootComponent);
	SetRootComponent(StaticMesh);

	Hitbox = CreateDefaultSubobject<UBoxComponent>(FName("Hit Box"));
	Hitbox->SetupAttachment(RootComponent);

	Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	Hitbox->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);

	PieceHealth = PieceMaxHealth;

	/*
	*	Foundation Sockets
	*/

	FoundationParent = CreateDefaultSubobject<USceneComponent>(FName("Foundation Socket Holder"));
	FoundationParent->SetupAttachment(RootComponent);

	FoundationSocket_1 = InitializeSocket(FName("Foundation Socket 1"), FoundationParent,  ECC_FoundationSocket);
	FoundationSocket_2 = InitializeSocket(FName("Foundation Socket 2"), FoundationParent, ECC_FoundationSocket);
	FoundationSocket_3 = InitializeSocket(FName("Foundation Socket 3"), FoundationParent, ECC_FoundationSocket);
	FoundationSocket_4 = InitializeSocket(FName("Foundation Socket 4"), FoundationParent, ECC_FoundationSocket);

	/*
	*	Wall Sockets
	*/

	WallParent = CreateDefaultSubobject<USceneComponent>(FName("Wall Socket Holder"));
	WallParent->SetupAttachment(RootComponent);

	WallSocket_1 = InitializeSocket(FName("Wall Socket 1"), WallParent, ECC_WallSocket);
	WallSocket_2 = InitializeSocket(FName("Wall Socket 2"), WallParent, ECC_WallSocket);
	WallSocket_3 = InitializeSocket(FName("Wall Socket 3"), WallParent, ECC_WallSocket);
	WallSocket_4 = InitializeSocket(FName("Wall Socket 4"), WallParent, ECC_WallSocket);

	/*
	*	Floor Sockets
	*/

	FloorParent = CreateDefaultSubobject<USceneComponent>(FName("Floor Socket Holder"));
	FloorParent->SetupAttachment(RootComponent);

	FloorSocket_1 = InitializeSocket(FName("Floor Socket 1"), FloorParent, ECC_FloorSocket);
	FloorSocket_2 = InitializeSocket(FName("Floor Socket 2"), FloorParent, ECC_FloorSocket);
	FloorSocket_3 = InitializeSocket(FName("Floor Socket 3"), FloorParent, ECC_FloorSocket);
	FloorSocket_4 = InitializeSocket(FName("Floor Socket 4"), FloorParent, ECC_FloorSocket);

	/*
	*	Ramp Sockets
	*/

	RampParent = CreateDefaultSubobject<USceneComponent>(FName("Ramp Socket Holder"));
	RampParent->SetupAttachment(RootComponent);

	RampSocket_1 = InitializeSocket(FName("Ramp Socket 1"), RampParent, ECC_RampSocket);
	RampSocket_2 = InitializeSocket(FName("Ramp Socket 2"), RampParent, ECC_RampSocket);
	RampSocket_3 = InitializeSocket(FName("Ramp Socket 3"), RampParent, ECC_RampSocket);
	RampSocket_4 = InitializeSocket(FName("Ramp Socket 4"), RampParent, ECC_RampSocket);
}

// Helper function to initialize foundation sockets
USocketComponent* ABuildingPieceBase::InitializeSocket(const FName& SocketName, USceneComponent* Holder,
	ECollisionChannel CollisionChannel)
{
	USocketComponent* Socket = CreateDefaultSubobject<USocketComponent>(SocketName);
	Socket->SetupAttachment(Holder);
	Socket->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Socket->SetCollisionResponseToAllChannels(ECR_Ignore);
	Socket->SetCollisionResponseToChannel(CollisionChannel, ECR_Block);
	return Socket;
}

void ABuildingPieceBase::BeginPlay()
{
	Super::BeginPlay();

	DefaultMaterials = StaticMesh->GetMaterials();
	for (int i = 0; i < DefaultMaterials.Num(); i++)
		StaticMesh->SetMaterial(i, PreviewMaterial);

	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Snapping Sockets
	SnappingSockets.AddUnique(FoundationSocket_1);
	SnappingSockets.AddUnique(FoundationSocket_2);
	SnappingSockets.AddUnique(FoundationSocket_3);
	SnappingSockets.AddUnique(FoundationSocket_4);

	SnappingSockets.AddUnique(WallSocket_1);
	SnappingSockets.AddUnique(WallSocket_2);
	SnappingSockets.AddUnique(WallSocket_3);
	SnappingSockets.AddUnique(WallSocket_4);

	SnappingSockets.AddUnique(FloorSocket_1);
	SnappingSockets.AddUnique(FloorSocket_2);
	SnappingSockets.AddUnique(FloorSocket_3);
	SnappingSockets.AddUnique(FloorSocket_4);

	SnappingSockets.AddUnique(RampSocket_1);
	SnappingSockets.AddUnique(RampSocket_2);
	SnappingSockets.AddUnique(RampSocket_3);
	SnappingSockets.AddUnique(RampSocket_4);
}

void ABuildingPieceBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABuildingPieceBase::Placed() const
{
	for (int i = 0; i < DefaultMaterials.Num(); i++)
		StaticMesh->SetMaterial(i, DefaultMaterials[i]);

	StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

bool ABuildingPieceBase::CheckIfBlocked()
{
	return false;
}

void ABuildingPieceBase::ToggleIncorrectMaterial(bool IncorrectLocation)
{
	if(IncorrectLocation)
	{
		for (int i = 0; i < DefaultMaterials.Num(); i++)
			StaticMesh->SetMaterial(i, IncorrectMaterial);
	}
	else
	{
		for (int i = 0; i < DefaultMaterials.Num(); i++)
			StaticMesh->SetMaterial(i, PreviewMaterial);
	}
}

void ABuildingPieceBase::Hit(const float Damage)
{
	PieceHealth -= Damage;

	if (PieceHealth.GetCurrentHealth() <= 0)
	{
		Destroy();
	}
}

TArray<USocketComponent*> ABuildingPieceBase::GetSnappingSockets() const { return SnappingSockets; }