#pragma once

#include "CoreMinimal.h"
#include "InteractableObject.h"
#include "PickupObject.generated.h"

UENUM(BlueprintType)
enum class EObjectState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_PickedUp UMETA(DisplayName = "Picked Up"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_Max UMETA(DisplayName = "Default Max")
};

UCLASS()
class TPP_BOILERPLATE_API APickupObject : public AInteractableObject
{
	GENERATED_BODY()

public:

	void TogglePhysics(bool Enabled) const;

private:


public:

	EObjectState CurrentObjectState;

};
