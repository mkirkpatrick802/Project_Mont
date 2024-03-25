#pragma once

#include "CoreMinimal.h"
#include "BuildInterface.h"
#include "GameFramework/Actor.h"
#include "BuildingPieceBase.generated.h"

#define ECC_FoundationSocket ECollisionChannel::ECC_GameTraceChannel3
#define ECC_FloorSocket ECollisionChannel::ECC_GameTraceChannel4
#define ECC_WallSocket ECollisionChannel::ECC_GameTraceChannel5

UCLASS()
class PROJECT_MONT_API ABuildingPieceBase : public AActor, public IBuildInterface
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* FoundationSocket_1;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* FoundationSocket_2;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* FoundationSocket_3;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* FoundationSocket_4;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* WallSocket_1;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* WallSocket_2;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* WallSocket_3;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* WallSocket_4;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* FloorSocket_1;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* FloorSocket_2;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* FloorSocket_3;

	UPROPERTY(EditAnywhere, Category = Sockets)
	USocketComponent* FloorSocket_4;

public:	

	ABuildingPieceBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void Placed() const;
	bool CheckIfBlocked();
	void ToggleIncorrectMaterial(bool IncorrectLocation);

	virtual TArray<USocketComponent*> GetSnappingSockets() const override;

private:

	USocketComponent* InitializeSocket(const FName& SocketName, ECollisionChannel CollisionChannel);

public:

	UPROPERTY(EditAnywhere, Category = "Building Settings")
	TArray<TEnumAsByte<ECollisionChannel>> TraceChannels;

protected:

	UPROPERTY(EditAnywhere, Category = "Building Preview")
	UMaterial* PreviewMaterial;

	UPROPERTY(EditAnywhere, Category = "Building Preview")
	UMaterial* IncorrectMaterial;

private:

	UPROPERTY()
	TArray<UMaterialInterface*> DefaultMaterials;

	TArray<USocketComponent*> SnappingSockets;
};
