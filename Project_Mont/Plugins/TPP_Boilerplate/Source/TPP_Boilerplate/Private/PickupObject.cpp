#include "PickupObject.h"

APickupObject::APickupObject()
{
	CurrentObjectState = EObjectState::EWS_Initial;
}

void APickupObject::TogglePhysics(bool Enabled) const
{
	ToggleInteractWidget(Enabled);

	UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(ModelComponent);
	USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(ModelComponent);

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
