#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Components/ActorComponent.h"
#include "BuildingComponent.generated.h"

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

	FTransform CheckForSnappingSockets(const FHitResult& Hit);

private:

	bool IsBuilding;
	bool PlacementBlocked;

	UPROPERTY()
	ACharacter* Character;

	UPROPERTY()
	ABuildingPieceBase* PreviewMesh;

	UPROPERTY()
	TSubclassOf<ABuildingPieceBase> CurrentBuildPiece;

	UPROPERTY(EditAnywhere, Category="Building Settings")
	float MaxBuildDistance = 500;

	FTransform PreviewTransform;

};
