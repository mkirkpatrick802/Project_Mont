#pragma once

#include "CoreMinimal.h"
#include "PickupObject.h"
#include "Components/ActorComponent.h"
#include "InteractComponent.generated.h"


class ATPPCharacter;
class AInteractableObject;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPP_BOILERPLATE_API UInteractComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = Interaction)
	class USphereComponent* InteractionSphere;

public:	

	UInteractComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void InteractRequest();

	UFUNCTION(BlueprintCallable)
	void InteractWithObject(AActor* ObjectToInteract);

	UFUNCTION(BlueprintCallable)
	void PickUpObjectRequest(APickupObject* ObjectToPickUp);

	void DropObjectRequest();

protected:

	virtual void BeginPlay() override;

private:

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void PickUpObject(APickupObject* ObjectToPickUp);

	UFUNCTION(Server, Reliable)
	void ServerPickUpObjectRequest(APickupObject* ObjectToPickUp);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPickUpObjectRequest(APickupObject* ObjectToPickUp);

	void DropObject();

	UFUNCTION(Server, Reliable)
	void ServerDropObjectRequest(APickupObject* ObjectToDrop);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDropObjectRequest();

	UFUNCTION(Server, Reliable)
	void ServerInteraction(AActor* Object, ATPPCharacter* Player);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastInteraction(AActor* Object, ATPPCharacter* Player);

public:

	UPROPERTY(Replicated)
	APickupObject* PickedUpObject;

private:

	class ATPPController* Controller;
	ATPPCharacter* Character;
};
