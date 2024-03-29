#include "InteractableObject.h"

#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

AInteractableObject::AInteractableObject()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	MeshComponent->SetupAttachment(RootComponent);
	SetRootComponent(MeshComponent);

	InteractWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Interact Widget"));
	InteractWidget->SetupAttachment(RootComponent);
	InteractWidget->SetVisibility(false);

	StaticMesh = Cast<UStaticMeshComponent>(MeshComponent);
	StaticMesh->SetCollisionObjectType(ECC_WorldDynamic);
	StaticMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	StaticMesh->SetIsReplicated(true);

	CurrentObjectState = EObjectState::EWS_NULL;
}

void AInteractableObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInteractableObject, CurrentObjectState)
}

FLinearColor AInteractableObject::GetColor() const
{
	return FLinearColor::Blue;
}

void AInteractableObject::OnRep_ObjectState()
{
	switch (CurrentObjectState)
	{
	case EObjectState::EWS_PickedUp:
		ToggleInteractWidget(false);

		break;
	case EObjectState::EWS_Dropped:

		break;
	default: ;
	}
}

void AInteractableObject::ToggleInteractWidget(const bool Enabled) const
{
	InteractWidget->SetVisibility(Enabled);
}

void AInteractableObject::SetObjectState(const EObjectState NewObjectState)
{
	CurrentObjectState = NewObjectState;

	switch (CurrentObjectState)
	{
	case EObjectState::EWS_PickedUp:
		ToggleInteractWidget(false);

		break;
	case EObjectState::EWS_Dropped:

		break;
	default:;
	}
}

void AInteractableObject::Interacted_Implementation(ATPPCharacter* Player) {}
