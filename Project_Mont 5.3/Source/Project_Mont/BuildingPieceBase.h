#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingPieceBase.generated.h"

UCLASS()
class PROJECT_MONT_API ABuildingPieceBase : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* StaticMesh;

public:	

	ABuildingPieceBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void Placed() const;

protected:

	UPROPERTY(EditAnywhere, Category = "Building")
	UMaterial* PreviewMaterial;

private:

	UPROPERTY()
	UMaterialInterface* DefaultMaterial;

};
