#include "PickupObject.h"

#include "InteractComponent.h"
#include "TPPCharacter.h"

APickupObject::APickupObject()
{
	CurrentObjectState = EObjectState::EWS_Initial;
}

void APickupObject::TogglePhysics(bool Enabled) const
{
	ToggleInteractWidget(Enabled);

	UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent);
	USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent);

	if (StaticMeshComponent)
	{
		StaticMeshComponent->SetSimulatePhysics(Enabled);
		StaticMeshComponent->SetAllPhysicsLinearVelocity(FVector::Zero());
	}

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetSimulatePhysics(Enabled);
		SkeletalMeshComponent->SetAllPhysicsLinearVelocity(FVector::Zero());
	}
}

void APickupObject::PickUp(ATPPCharacter* Player)
{
	if (const auto InteractComponent = Player->GetInteractComponent())
	{
		InteractComponent->PickUpObject(this);
	}
}
