#pragma once

#include "CoreMinimal.h"
#include "InteractInterface.h"
#include "GameFramework/Actor.h"
#include "InteractableObject.generated.h"

class UWidgetComponent;

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

	UFUNCTION(BlueprintNativeEvent)
	void Interacted(ATPPCharacter* Player);

	void ToggleInteractWidget(bool Enabled) const;
};
