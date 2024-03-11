#include "InteractableObject.h"

#include "Components/WidgetComponent.h"

AInteractableObject::AInteractableObject()
{
	PrimaryActorTick.bCanEverTick = true;

	ModelComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	ModelComponent->SetupAttachment(RootComponent);
	SetRootComponent(ModelComponent);

	InteractWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Interact Widget"));
	InteractWidget->SetupAttachment(RootComponent);
	InteractWidget->SetVisibility(false);

	StaticMesh = Cast<UStaticMeshComponent>(ModelComponent);
	StaticMesh->SetCollisionObjectType(ECC_WorldDynamic);
	StaticMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	StaticMesh->SetIsReplicated(true);
}

void AInteractableObject::ToggleInteractWidget(const bool Enabled) const
{
	InteractWidget->SetVisibility(Enabled);
}

void AInteractableObject::Interacted_Implementation(ATPPCharacter* Player) {}
