#pragma once

#include "CoreMinimal.h"
#include "InteractableObject.h"
#include "PickupObject.generated.h"

UCLASS()
class TPP_BOILERPLATE_API APickupObject : public AInteractableObject
{
	GENERATED_BODY()

public:

	APickupObject();
	void TogglePhysics(bool Enabled) const;

protected:

	UFUNCTION(BlueprintCallable)
	virtual void PickUp(ATPPCharacter* Player);

public:

	FORCEINLINE UMeshComponent* GetMeshComponent() const { return MeshComponent; }

};
