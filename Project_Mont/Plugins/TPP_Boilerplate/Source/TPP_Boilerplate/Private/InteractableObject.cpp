#include "InteractableObject.h"

#include "Components/WidgetComponent.h"

AInteractableObject::AInteractableObject()
{
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	StaticMesh->SetupAttachment(RootComponent);
	SetRootComponent(StaticMesh);

	InteractWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Interact Widget"));
	InteractWidget->SetupAttachment(RootComponent);
	InteractWidget->SetVisibility(false);

	StaticMesh->SetCollisionObjectType(ECC_WorldDynamic);
	StaticMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	StaticMesh->SetIsReplicated(true);
}

void AInteractableObject::ToggleInteractWidget(const bool Enabled) const
{
	InteractWidget->SetVisibility(Enabled);
}

void AInteractableObject::Interacted_Implementation(ATPPCharacter* Player) {}
