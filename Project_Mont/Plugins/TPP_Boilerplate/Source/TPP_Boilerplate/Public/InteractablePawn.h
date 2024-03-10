#pragma once

#include "CoreMinimal.h"
#include "InteractableObject.h"
#include "GameFramework/Pawn.h"
#include "InteractablePawn.generated.h"

UCLASS()
class TPP_BOILERPLATE_API AInteractablePawn : public AInteractableObject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

public:

	AInteractablePawn();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
};
