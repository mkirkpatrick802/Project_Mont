#include "PickupObject.h"

void APickupObject::TogglePhysics(bool Enabled) const
{
	ToggleInteractWidget(Enabled);
	StaticMesh->SetSimulatePhysics(Enabled);
	RootComponent->SetWorldRotation(FRotator::ZeroRotator);
}