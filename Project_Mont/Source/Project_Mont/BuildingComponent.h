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
	class UInputMappingContext* BuildingMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* BuildModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* PlaceAction;

public:

	UBuildingComponent();

	virtual void BeginPlay() override;
	void SetupInput();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	UFUNCTION()
	void ToggleBuildMode(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void PlacePiece(const FInputActionValue& InputActionValue);

private:

	bool IsBuilding = false;

	UPROPERTY()
	ACharacter* Character;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ABuildingPieceBase> CurrentBuildPiece;

	UPROPERTY(EditAnywhere, Category="Building Settings")
	float MaxBuildDistance;

	FVector PreviewLocation;

};
