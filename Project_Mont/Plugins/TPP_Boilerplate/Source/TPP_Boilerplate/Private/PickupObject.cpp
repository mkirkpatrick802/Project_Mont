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

	if (const auto RootPrimitiveComponent = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		RootPrimitiveComponent->SetSimulatePhysics(Enabled);
		RootPrimitiveComponent->SetAllPhysicsLinearVelocity(FVector::Zero());
	}
}

void APickupObject::PickUp(ATPPCharacter* Player)
{
	if (const auto InteractComponent = Player->GetInteractComponent())
	{
		InteractComponent->PickUpObject(this);
	}
}
