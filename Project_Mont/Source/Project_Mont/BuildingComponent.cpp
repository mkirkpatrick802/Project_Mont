#include "BuildingComponent.h"

#include "BuildingPieceBase.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "CrosshairUtility.h"
#include "SocketComponent.h"
#include "SocketInterface.h"
#include "TPPCharacter.h"
#include "Components/BoxComponent.h"

UBuildingComponent::UBuildingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuildingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
		Character = Owner;

	CurrentBuildPiece = nullptr;
	IsBuilding = false;

	SetupInput();
}

void UBuildingComponent::SetupInput()
{
	if(!Character) return;

	if(const APlayerController* Controller = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(PlayerStateMappingContext, 1);
		}

		if(UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(Controller->InputComponent))
		{
			EnhancedInputComponent->BindAction(BuildModeAction, ETriggerEvent::Started, this, &UBuildingComponent::ToggleBuildModeInput);
		}
	}
}

void UBuildingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(!IsBuilding || !CurrentBuildPiece) return;

	if (!PreviewMesh)
		PreviewMesh = GetWorld()->SpawnActor<ABuildingPieceBase>(CurrentBuildPiece);

	ResetPlacementBlocked();
	InSocket = false;

	if(const auto TPPCharacter = Cast<ATPPCharacter>(Character))
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(TPPCharacter);
		QueryParams.AddIgnoredActor(PreviewMesh);

		const float OffsetLength = 100;

		//TODO: Use Multiple Channels
		CrosshairUtility::TraceUnderCrosshairs(TPPCharacter, HitResult, OffsetLength, MaxBuildDistance + (OffsetLength/2), PreviewMesh->TraceChannels[0], QueryParams);

		if (HitResult.GetActor())
		{
			CurrentSocket = CheckForSnappingSockets(HitResult);
			if(CurrentSocket)
			{
				PreviewTransform = CurrentSocket->GetComponentTransform();
			}
			else
			{
				PreviewTransform = FTransform(FRotator::ZeroRotator, HitResult.ImpactPoint);
			}

			PreviewMesh->SetActorTransform(PreviewTransform);
		}
		else
		{
			PreviewMesh->Destroy();
			PreviewMesh = nullptr;
		}
	}

	const ABuildingPieceBase* BuildPiece = CurrentBuildPiece.GetDefaultObject();
	SetPlacementBlocked(BuildPiece->Cost > CurrentResources);
	SetPlacementBlocked(BuildPiece->NeedsSocket && !InSocket);

	if (PreviewMesh)
		PreviewMesh->ToggleIncorrectMaterial(PlacementBlocked);
}

void UBuildingComponent::BuildPieceSelected(TSubclassOf<ABuildingPieceBase> SelectedPiece)
{
	CurrentBuildPiece = SelectedPiece;
	if (PreviewMesh)
	{
		PreviewMesh->Destroy();
		PreviewMesh = nullptr;
	}

	ToggleBuildMode(true);
}

void UBuildingComponent::ToggleBuildModeInput(const FInputActionValue& InputActionValue)
{
	if (!CurrentBuildPiece) return;
	ToggleBuildMode(!IsBuilding);
}

void UBuildingComponent::ToggleBuildMode(bool Value)
{
	IsBuilding = Value;

	if (!IsBuilding && PreviewMesh)
	{
		PreviewMesh->Destroy();
		PreviewMesh = nullptr;
	}

	if (IsBuilding)
	{
		if (const APlayerController* Controller = Cast<APlayerController>(Character->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()))
			{
				Subsystem->AddMappingContext(BuildingMappingContext, 1);
			}

			if (!HaveActionsBeenBinded)
			{
				if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(Controller->InputComponent))
				{
					PlaceAction->bConsumeInput = true;
					EnhancedInputComponent->BindAction(PlaceAction, ETriggerEvent::Started, this, &UBuildingComponent::PlacePiece);
				}

				HaveActionsBeenBinded = true;
			}
		}
	}
	else
	{
		if (const APlayerController* Controller = Cast<APlayerController>(Character->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()))
			{
				Subsystem->RemoveMappingContext(BuildingMappingContext);
			}
		}
	}
}

void UBuildingComponent::PlacePiece(const FInputActionValue& InputActionValue)
{
	if(PlacementBlocked) return;
	if(!IsBuilding || !CurrentBuildPiece) return;
	if(PreviewTransform.GetLocation() == FVector::Zero()) return;

	const ABuildingPieceBase* BuildPiece = CurrentBuildPiece.GetDefaultObject();
	if(BuildPiece->Cost > CurrentResources) return;
	CurrentResources -= BuildPiece->Cost;

	const auto SpawnedPiece = GetWorld()->SpawnActor<ABuildingPieceBase>(CurrentBuildPiece, PreviewTransform);
	SpawnedPiece->Placed();
}

USocketComponent* UBuildingComponent::CheckForSnappingSockets(const FHitResult& Hit)
{
	if (!Hit.GetActor()) return nullptr;
	if (!Hit.GetActor()->Implements<UBuildInterface>()) return nullptr;

	const ABuildingPieceBase* Piece = Cast<ABuildingPieceBase>(Hit.GetActor());

	const USocketComponent* Check = Cast<USocketComponent>(Hit.GetComponent());
	SetPlacementBlocked(Check == nullptr);

	TArray<USocketComponent*> Sockets = Piece->GetSnappingSockets();
	for (USocketComponent* Socket : Sockets)
	{
		if(Cast<UPrimitiveComponent>(Socket) != Hit.GetComponent()) continue;
		InSocket = true;
		return Socket;
	}

	return nullptr;
}

void UBuildingComponent::GetOtherSockets(const FVector& HitLocation, TArray<USocketComponent*>& Sockets)
{
	const UWorld* World = GetWorld();
	if (!World) return;

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true; // Or false, depending on your needs
	QueryParams.AddIgnoredActor(nullptr); // Add any actors you want to ignore during the query

	// Define the shape of your overlap. In this case, a sphere
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(7);

	// Perform the overlap query
	const bool bHasOverlap = World->OverlapMultiByChannel(
		OverlapResults,
		HitLocation,
		FQuat::Identity,
		PreviewMesh->TraceChannels[0], // Use the trace channel that suits your scenario
		CollisionShape,
		QueryParams
	);

	if (bHasOverlap)
	{
		int key = 10;
		for (const FOverlapResult& Result : OverlapResults)
		{
			if (UPrimitiveComponent* OverlappedComponent = Result.GetComponent())
			{
				if(!OverlappedComponent->Implements<USocketInterface>()) continue;

				Sockets.AddUnique(Cast<USocketComponent>(OverlappedComponent));
			}
			key++;
		}
	}
}

void UBuildingComponent::SetPlacementBlocked(const bool Blocked)
{
	if(PlacementBlocked) return;
	PlacementBlocked = Blocked;
}

void UBuildingComponent::ResetPlacementBlocked()
{
	PlacementBlocked = false;
}
