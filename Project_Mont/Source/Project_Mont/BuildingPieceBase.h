#pragma once

#include "CoreMinimal.h"
#include "BuildInterface.h"
#include "DamageableInterface.h"
#include "Health.h"
#include "GameFramework/Actor.h"
#include "BuildingPieceBase.generated.h"

#define ECC_FoundationSocket ECollisionChannel::ECC_GameTraceChannel3
#define ECC_FloorSocket ECollisionChannel::ECC_GameTraceChannel4
#define ECC_WallSocket ECollisionChannel::ECC_GameTraceChannel5
#define ECC_RampSocket ECollisionChannel::ECC_GameTraceChannel7

class UBoxComponent;

UCLASS()
class PROJECT_MONT_API ABuildingPieceBase : public AActor, public IBuildInterface, public IDamageableInterface
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditAnywhere, Category = Combat)
	UBoxComponent* Hitbox;

	/*
	*	Foundation Sockets
	*/

	UPROPERTY(EditAnywhere, Category = Foundation)
	USceneComponent* FoundationParent;

	UPROPERTY(EditAnywhere, Category = Foundation)
	USocketComponent* FoundationSocket_1;

	UPROPERTY(EditAnywhere, Category = Foundation)
	USocketComponent* FoundationSocket_2;

	UPROPERTY(EditAnywhere, Category = Foundation)
	USocketComponent* FoundationSocket_3;

	UPROPERTY(EditAnywhere, Category = Foundation)
	USocketComponent* FoundationSocket_4;

	/*
	*	Wall Sockets
	*/

	UPROPERTY(EditAnywhere, Category = Wall)
	USceneComponent* WallParent;

	UPROPERTY(EditAnywhere, Category = Wall)
	USocketComponent* WallSocket_1;

	UPROPERTY(EditAnywhere, Category = Wall)
	USocketComponent* WallSocket_2;

	UPROPERTY(EditAnywhere, Category = Wall)
	USocketComponent* WallSocket_3;

	UPROPERTY(EditAnywhere, Category = Wall)
	USocketComponent* WallSocket_4;

	/*
	*	Floor Sockets
	*/

	UPROPERTY(EditAnywhere, Category = Floor)
	USceneComponent* FloorParent;

	UPROPERTY(EditAnywhere, Category = Floor)
	USocketComponent* FloorSocket_1;

	UPROPERTY(EditAnywhere, Category = Floor)
	USocketComponent* FloorSocket_2;

	UPROPERTY(EditAnywhere, Category = Floor)
	USocketComponent* FloorSocket_3;

	UPROPERTY(EditAnywhere, Category = Floor)
	USocketComponent* FloorSocket_4;

	/*
	*	Ramp Sockets
	*/

	UPROPERTY(EditAnywhere, Category = Ramp)
	USceneComponent* RampParent;

	UPROPERTY(EditAnywhere, Category = Ramp)
	USocketComponent* RampSocket_1;

	UPROPERTY(EditAnywhere, Category = Ramp)
	USocketComponent* RampSocket_2;

	UPROPERTY(EditAnywhere, Category = Ramp)
	USocketComponent* RampSocket_3;

	UPROPERTY(EditAnywhere, Category = Ramp)
	USocketComponent* RampSocket_4;

public:	

	ABuildingPieceBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category=Building)
	void Placed() const;


	bool CheckIfBlocked();
	void ToggleIncorrectMaterial(bool IncorrectLocation);

	virtual void Hit(float Damage) override;

	virtual TArray<USocketComponent*> GetSnappingSockets() const override;

private:

	USocketComponent* InitializeSocket(const FName& SocketName, USceneComponent* Holder, ECollisionChannel CollisionChannel);

public:

	UPROPERTY(EditAnywhere, Category = "Building Settings")
	TArray<TEnumAsByte<ECollisionChannel>> TraceChannels;

	UPROPERTY(EditAnywhere, Category = "Piece Properties")
	bool NeedsSocket = true;

	UPROPERTY(EditAnywhere, Category = "Piece Properties")
	int Cost = 2;

protected:

	UPROPERTY(EditAnywhere, Category = "Piece Properties")
	int PieceMaxHealth = 100;

	UPROPERTY(EditAnywhere, Category = "Building Preview")
	UMaterial* PreviewMaterial;

	UPROPERTY(EditAnywhere, Category = "Building Preview")
	UMaterial* IncorrectMaterial;

private:

	UPROPERTY()
	TArray<UMaterialInterface*> DefaultMaterials;

	TArray<USocketComponent*> SnappingSockets;

	Health PieceHealth;
};
