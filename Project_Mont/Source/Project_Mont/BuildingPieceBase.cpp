#include "BuildingPieceBase.h"

#include "SocketComponent.h"

ABuildingPieceBase::ABuildingPieceBase()
{
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static Mesh"));
	StaticMesh->SetupAttachment(RootComponent);
	SetRootComponent(StaticMesh);

	StaticMesh->SetCollisionResponseToAllChannels(ECR_Overlap);

	FoundationSocket_1 = InitializeSocket(FName("Foundation Socket 1"), ECC_FoundationSocket);
	FoundationSocket_2 = InitializeSocket(FName("Foundation Socket 2"), ECC_FoundationSocket);
	FoundationSocket_3 = InitializeSocket(FName("Foundation Socket 3"), ECC_FoundationSocket);
	FoundationSocket_4 = InitializeSocket(FName("Foundation Socket 4"), ECC_FoundationSocket);

	WallSocket_1 = InitializeSocket(FName("Wall Socket 1"), ECC_WallSocket);
	WallSocket_2 = InitializeSocket(FName("Wall Socket 2"), ECC_WallSocket);
	WallSocket_3 = InitializeSocket(FName("Wall Socket 3"), ECC_WallSocket);
	WallSocket_4 = InitializeSocket(FName("Wall Socket 4"), ECC_WallSocket);

	FloorSocket_1 = InitializeSocket(FName("Floor Socket 1"), ECC_FloorSocket);
	FloorSocket_2 = InitializeSocket(FName("Floor Socket 2"), ECC_FloorSocket);
	FloorSocket_3 = InitializeSocket(FName("Floor Socket 3"), ECC_FloorSocket);
	FloorSocket_4 = InitializeSocket(FName("Floor Socket 4"), ECC_FloorSocket);
}

// Helper function to initialize foundation sockets
USocketComponent* ABuildingPieceBase::InitializeSocket(const FName& SocketName, const ECollisionChannel CollisionChannel)
{
	USocketComponent* Socket = CreateDefaultSubobject<USocketComponent>(SocketName);
	Socket->SetupAttachment(RootComponent);
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

TArray<USocketComponent*> ABuildingPieceBase::GetSnappingSockets() const { return SnappingSockets; }