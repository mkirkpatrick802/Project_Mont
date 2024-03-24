#include "InteractComponent.h"

#include "CombatComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InteractableObject.h"
#include "InteractInterface.h"
#include "TPPCharacter.h"
#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UInteractComponent::UInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Sphere"));
}

void UInteractComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInteractComponent, PickedUpObject)
}

void UInteractComponent::ToggleHeldObjectVisibility(bool IsVisible)
{
	if (PickedUpObject)
	{
		PickedUpObject->GetMeshComponent()->bOwnerNoSee = !IsVisible;
	}
}

void UInteractComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ATPPCharacter>(GetOwner());

	SetupComponents();

	Character->SetupPlayerInputDelegate.AddDynamic(this, &UInteractComponent::SetupInput);
}

void UInteractComponent::SetupComponents()
{

	InteractionSphere->RegisterComponent();
	InteractionSphere->AttachToComponent(Character->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionObjectType(ECC_PhysicsBody);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	InteractionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetGenerateOverlapEvents(true);

	if (InteractionSphere)
	{
		InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &UInteractComponent::OnInteractionSphereBeginOverlap);
		InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &UInteractComponent::OnInteractionSphereEndOverlap);
	}
}

void UInteractComponent::SetupInput()
{
	if(const auto InputSubsystem = Character->GetInputSubsystem())
	{
		InputSubsystem->AddMappingContext(InteractionMappingContext, 0);
	}

	if(const auto InputComponent = Character->GetInputComponent())
	{
		InputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &UInteractComponent::InteractRequest);
		InputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &UInteractComponent::DropObjectRequest);
	}
}

/*
 *	Interact With Object
 */

void UInteractComponent::InteractRequest()
{
	TArray<AActor*> OverlappingActors;
	InteractionSphere->GetOverlappingActors(OverlappingActors);
	for (const auto OverlappingActor : OverlappingActors)
	{
		if (!OverlappingActor->Implements<UInteractInterface>()) continue;

		if (const AInteractableObject* InteractableObject = Cast<AInteractableObject>(OverlappingActor))
			if (InteractableObject->CurrentObjectState == EObjectState::EWS_PickedUp) continue;

		InteractWithObject(OverlappingActor);
		break;
	}
}

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
	if (AInteractableObject* Actor = Cast<AInteractableObject>(Object))
	{
		Actor->Interacted(Player);
	}
}

/*
 *	Pick Up Object
 */

void UInteractComponent::PickUpObject(APickupObject* ObjectToPickUp)
{
	if(ObjectToPickUp->CurrentObjectState == EObjectState::EWS_PickedUp) return;
	if(PickedUpObject) return;

	ServerPickUpObjectRequest(ObjectToPickUp);
}

void UInteractComponent::ServerPickUpObjectRequest_Implementation(APickupObject* ObjectToPickUp)
{
	MulticastPickUpObjectRequest(ObjectToPickUp);
}

void UInteractComponent::MulticastPickUpObjectRequest_Implementation(APickupObject* ObjectToPickUp)
{
	if (!Character || !ObjectToPickUp) return;

	PickedUpObject = ObjectToPickUp;
	PickedUpObject->CurrentObjectState = EObjectState::EWS_PickedUp;
	PickedUpObject->TogglePhysics(false);
	PickedUpObject->SetOwner(Character);

	if (const auto Weapon = Cast<AWeapon>(PickedUpObject))
	{
		const auto CombatComponent = Character->GetCombatComponent();
		CombatComponent->EquipWeapon(Weapon);
	}
	else
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed / 1.5;

		if (const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")))
			HandSocket->AttachActor(PickedUpObject, Character->GetMesh());
	}
}

/*
 *	Drop Object
 */

void UInteractComponent::DropObjectRequest()
{
	if(!PickedUpObject) return;
	ServerDropObjectRequest(PickedUpObject);
}

void UInteractComponent::ServerDropObjectRequest_Implementation(APickupObject* ObjectToDrop)
{
	MulticastDropObjectRequest();
}

void UInteractComponent::MulticastDropObjectRequest_Implementation()
{
	if (!PickedUpObject) return;
	PickedUpObject->CurrentObjectState = EObjectState::EWS_Dropped;
	PickedUpObject->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	PickedUpObject->TogglePhysics(true);
	PickedUpObject->SetOwner(nullptr);
	PickedUpObject = nullptr;

	Character->GetCharacterMovement()->MaxWalkSpeed = 600;

	const auto CombatComponent = Character->GetCombatComponent();
	CombatComponent->DropWeapon();
}

/*
 *	Overlaps
 */

void UInteractComponent::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->Implements<UInteractInterface>()) return;

	if (const AInteractableObject* InteractableObject = Cast<AInteractableObject>(OtherActor))
	{
		if (InteractableObject->CurrentObjectState == EObjectState::EWS_PickedUp) return;

		if (Character->IsLocallyControlled())
			InteractableObject->ToggleInteractWidget(true);
	}
}

void UInteractComponent::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor->Implements<UInteractInterface>()) return;

	if (const AInteractableObject* InteractableObject = Cast<AInteractableObject>(OtherActor))
	{
		if (Character->IsLocallyControlled())
			InteractableObject->ToggleInteractWidget(false);
	}
}