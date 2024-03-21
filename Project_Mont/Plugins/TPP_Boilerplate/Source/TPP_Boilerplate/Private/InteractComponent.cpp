#include "InteractComponent.h"

#include "CombatComponent.h"
#include "InteractableObject.h"
#include "InteractablePawn.h"
#include "InteractInterface.h"
#include "TPPCharacter.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UInteractComponent::UInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Sphere"));
}

void UInteractComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInteractComponent, PickedUpObject)
}

void UInteractComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ATPPCharacter>(GetOwner());

	InteractionSphere->RegisterComponent();
	InteractionSphere->AttachToComponent(Character->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	InteractionSphere->SetCollisionObjectType(ECC_WorldStatic);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	InteractionSphere->SetSphereRadius(100);
	InteractionSphere->SetGenerateOverlapEvents(true);

	if(InteractionSphere)
	{
		InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &UInteractComponent::OnInteractionSphereBeginOverlap);
		InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &UInteractComponent::OnInteractionSphereEndOverlap);
	}
}

void UInteractComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

/*
 *		Object Pick Up
 */

void UInteractComponent::PickUpObjectRequest(APickupObject* ObjectToPickUp)
{
	if(ObjectToPickUp->CurrentObjectState == EObjectState::EWS_PickedUp) return;

	ServerPickUpObjectRequest(ObjectToPickUp);
}

void UInteractComponent::ServerPickUpObjectRequest_Implementation(APickupObject* ObjectToPickUp)
{
	MulticastPickUpObjectRequest(ObjectToPickUp);
}

void UInteractComponent::MulticastPickUpObjectRequest_Implementation(APickupObject* ObjectToPickUp)
{
	PickUpObject(ObjectToPickUp);
}

void UInteractComponent::PickUpObject(APickupObject* ObjectToPickUp)
{
	if (!Character || !ObjectToPickUp || PickedUpObject) return;

	PickedUpObject = ObjectToPickUp;
	PickedUpObject->CurrentObjectState = EObjectState::EWS_PickedUp;
	PickedUpObject->TogglePhysics(false);

	Character->GetCharacterMovement()->MaxWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed / 1.5;

	if (const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		HandSocket->AttachActor(PickedUpObject, Character->GetMesh());
	}

	PickedUpObject->SetOwner(Character);
}

void UInteractComponent::DropObjectRequest()
{
	ServerDropObjectRequest(PickedUpObject);
}

void UInteractComponent::ServerDropObjectRequest_Implementation(APickupObject* ObjectToDrop)
{
	MulticastDropObjectRequest();
}

void UInteractComponent::MulticastDropObjectRequest_Implementation()
{
	DropObject();
}

void UInteractComponent::DropObject()
{
	if(PickedUpObject)
	{
		PickedUpObject->CurrentObjectState = EObjectState::EWS_Dropped;
		PickedUpObject->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		PickedUpObject->TogglePhysics(true);
		PickedUpObject->SetOwner(nullptr);
		PickedUpObject = nullptr;

		Character->GetCharacterMovement()->MaxWalkSpeed = 600;
	}
	else
	{
		if(UCombatComponent* Combat = Character->GetCombatComponent())
		{
			Combat->DropWeapon();
		}
	}
}

/*
 *		Object Interaction
 */

// Not Replicated
void UInteractComponent::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->Implements<UInteractInterface>()) return;

	if (const AInteractableObject* InteractableObject = Cast<AInteractableObject>(OtherActor))
	{
		if(InteractableObject->CurrentObjectState == EObjectState::EWS_PickedUp) return;

		if(Character->IsLocallyControlled())
			InteractableObject->ToggleInteractWidget(true);
	}

	IInteractInterface::Execute_EnteredInteractionZone(OtherActor, Character);
}

// Not Replicated
void UInteractComponent::InteractRequest()
{
	TArray<AActor*> OverlappingActors;
	InteractionSphere->GetOverlappingActors(OverlappingActors);
	for (const auto OverlappingActor : OverlappingActors)
	{
		if (!OverlappingActor->Implements<UInteractInterface>()) continue;

		if (const AInteractableObject* InteractableObject = Cast<AInteractableObject>(OverlappingActor))
			if (InteractableObject->CurrentObjectState == EObjectState::EWS_PickedUp) continue;

		IInteractInterface::Execute_InteractRequest(OverlappingActor, Character);
		break;
	}
}

// Not Replicated
void UInteractComponent::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor->Implements<UInteractInterface>()) return;

	if (const AInteractableObject* InteractableObject = Cast<AInteractableObject>(OtherActor))
	{
		if (Character->IsLocallyControlled())
			InteractableObject->ToggleInteractWidget(false);
	}

	IInteractInterface::Execute_LeftInteractionZone(OtherActor, Character);
}

/*
 *	Interaction Chain
 */

void UInteractComponent::InteractWithObject(AActor* ObjectToInteract)
{
	ServerInteraction(ObjectToInteract, Character);
}

void UInteractComponent::ServerInteraction_Implementation(AActor* Object, ATPPCharacter* Player)
{
	MulticastInteraction(Object, Player);
}

void UInteractComponent::MulticastInteraction_Implementation(AActor* Object, ATPPCharacter* Player)
{
	if(AInteractableObject* Actor = Cast<AInteractableObject>(Object))
	{
		Actor->Interacted(Player);
	}

	if (AInteractablePawn* Pawn = Cast<AInteractablePawn>(Object))
	{
		Pawn->Interacted(Player);
	}
}