#pragma once

#include "CoreMinimal.h"
#include "InteractInterface.h"
#include "GameFramework/Actor.h"
#include "InteractableObject.generated.h"

class UWidgetComponent;

UENUM(BlueprintType)
enum class EObjectState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_PickedUp UMETA(DisplayName = "Picked Up"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_NULL UMETA(DisplayName = "Can't Be Picked Up"),
	EWS_Max UMETA(DisplayName = "Default Max")
};

UCLASS()
class TPP_BOILERPLATE_API AInteractableObject : public APawn, public IInteractInterface
{
	GENERATED_BODY()

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* ModelComponent;

	UPROPERTY()
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UWidgetComponent* InteractWidget;

public:	

	AInteractableObject();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintNativeEvent)
	void Interacted(ATPPCharacter* Player);

	void ToggleInteractWidget(bool Enabled) const;

	void SetObjectState(EObjectState NewObjectState);

	// For Crosshair Interaction
	virtual FLinearColor GetColor() const override;

protected:

	UFUNCTION()
	void OnRep_ObjectState();

public:

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_ObjectState, Category = "Object Properties")
	EObjectState CurrentObjectState;

};