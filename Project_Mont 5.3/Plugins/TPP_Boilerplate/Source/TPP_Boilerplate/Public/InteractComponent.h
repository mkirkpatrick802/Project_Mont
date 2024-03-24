#pragma once

#include "CoreMinimal.h"
#include "PickupObject.h"
#include "Components/ActorComponent.h"
#include "InteractComponent.generated.h"


class UInputAction;
class UInputMappingContext;
class USphereComponent;
class ATPPCharacter;
class AInteractableObject;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPP_BOILERPLATE_API UInteractComponent : public UActorComponent
{
	GENERATED_BODY()

	/*
	*	Components
	*/

	UPROPERTY(VisibleAnywhere, Category = Interaction)
	USphereComponent* InteractionSphere;

	/*
	*	Inputs
	*/

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* InteractionMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* DropAction;

public:	

	UInteractComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ToggleHeldObjectVisibility(bool IsVisible);

	void PickUpObject(APickupObject* ObjectToPickUp);

protected:

	virtual void BeginPlay() override;

private:

	void SetupComponents();

	UFUNCTION()
	void SetupInput();

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/*
	*	Interaction
	*/

	void InteractRequest();

	void InteractWithObject(AActor* ObjectToInteract);

	UFUNCTION(Server, Reliable)
	void ServerInteraction(AActor* Object, ATPPCharacter* Player);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastInteraction(AActor* Object, ATPPCharacter* Player);

	/*
	*	Pick Up
	*/

	UFUNCTION(Server, Reliable)
	void ServerPickUpObjectRequest(APickupObject* ObjectToPickUp);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPickUpObjectRequest(APickupObject* ObjectToPickUp);

	/*
	*	Drop
	*/

	void DropObjectRequest();

	UFUNCTION(Server, Reliable)
	void ServerDropObjectRequest(APickupObject* ObjectToDrop);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDropObjectRequest();

public:

	UPROPERTY(Replicated)
	APickupObject* PickedUpObject;

private:

	UPROPERTY(EditAnywhere, Category=Interaction)
	float InteractionRadius = 200;

	class ATPPController* Controller;
	ATPPCharacter* Character;

public:

	FORCEINLINE bool IsHoldingObject() const { return PickedUpObject != nullptr; }
	FORCEINLINE APickupObject* GetHeldObject() const { return PickedUpObject; }

};
