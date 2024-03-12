#include "PickupObject.h"

APickupObject::APickupObject()
{
	CurrentObjectState = EObjectState::EWS_Initial;
}

void APickupObject::TogglePhysics(bool Enabled) const
{
	ToggleInteractWidget(Enabled);
	StaticMesh->SetSimulatePhysics(Enabled);
	RootComponent->SetWorldRotation(FRotator::ZeroRotator);
}
