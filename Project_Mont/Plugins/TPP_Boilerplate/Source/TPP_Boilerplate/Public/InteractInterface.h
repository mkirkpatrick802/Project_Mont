#pragma once

#include "CoreMinimal.h"
#include "InteractWithCrosshairsInterface.h"
#include "UObject/Interface.h"
#include "InteractInterface.generated.h"

class ATPPCharacter;

UINTERFACE(MinimalAPI)
class UInteractInterface : public UInteractWithCrosshairsInterface
{
	GENERATED_BODY()
};

class TPP_BOILERPLATE_API IInteractInterface : public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interact")
	void EnteredInteractionZone(ATPPCharacter* Player);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interact")
	void LeftInteractionZone(ATPPCharacter* Player);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interact")
	void InteractRequest(ATPPCharacter* Player);

};
