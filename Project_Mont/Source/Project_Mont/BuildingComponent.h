#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Components/ActorComponent.h"
#include "BuildingComponent.generated.h"

class USocketComponent;
class UBoxComponent;
class ABuildingPieceBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))

class PROJECT_MONT_API UBuildingComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* PlayerStateMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* BuildModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* BuildingMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* PlaceAction;

public:

	UBuildingComponent();

	virtual void BeginPlay() override;
	void SetupInput();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category=Building)
	void BuildPieceSelected(TSubclassOf<ABuildingPieceBase> SelectedPiece);

private:

	UFUNCTION()
	void ToggleBuildModeInput(const FInputActionValue& InputActionValue);
	void ToggleBuildMode(bool Value);

	UFUNCTION()
	void PlacePiece(const FInputActionValue& InputActionValue);

	USocketComponent* CheckForSnappingSockets(const FHitResult& Hit);
	void GetOtherSockets(const FVector& HitLocation, TArray<USocketComponent*>& Sockets);

	void SetPlacementBlocked(const bool Blocked);
	void ResetPlacementBlocked();

protected:

	UPROPERTY(BlueprintReadWrite, Category = "Resources")
	int CurrentResources = 0;

private:

	bool InSocket = false;
	bool HaveActionsBeenBinded = false;

	bool IsBuilding;
	bool PlacementBlocked = false;

	UPROPERTY()
	ACharacter* Character;

	UPROPERTY()
	ABuildingPieceBase* PreviewMesh;

	UPROPERTY()
	TSubclassOf<ABuildingPieceBase> CurrentBuildPiece;

	UPROPERTY(EditAnywhere, Category = "Building Settings")
	float MaxBuildDistance = 500;

	USocketComponent* CurrentSocket;
	FTransform PreviewTransform;

};
