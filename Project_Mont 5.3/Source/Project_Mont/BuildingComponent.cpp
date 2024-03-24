#include "BuildingComponent.h"

#include "BuildingPieceBase.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "CrosshairUtility.h"
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

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(2, 15.0f, FColor::Red, FString::Printf(TEXT("Consume Input: %d"), PlaceAction->bConsumeInput));

	if(!IsBuilding || !CurrentBuildPiece) return;

	if (!PreviewMesh)
		PreviewMesh = GetWorld()->SpawnActor<ABuildingPieceBase>(CurrentBuildPiece);

	if(const auto TPPCharacter = Cast<ATPPCharacter>(Character))
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(TPPCharacter);
		QueryParams.AddIgnoredActor(PreviewMesh);

		const float OffsetLength = 300;

		//TODO: Use Multiple Channels
		CrosshairUtility::TraceUnderCrosshairs(TPPCharacter, HitResult, OffsetLength, MaxBuildDistance + (OffsetLength/2), PreviewMesh->TraceChannels[0], QueryParams);

		if (HitResult.GetActor())
		{
			PreviewTransform = CheckForSnappingSockets(HitResult);;
			PreviewMesh->SetActorTransform(PreviewTransform);
		}
		else
		{
			PreviewMesh->Destroy();
			PreviewMesh = nullptr;
		}
	}
}

void UBuildingComponent::BuildPieceSelected(TSubclassOf<ABuildingPieceBase> SelectedPiece)
{
	CurrentBuildPiece = SelectedPiece;
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

			if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(Controller->InputComponent))
			{
				PlaceAction->bConsumeInput = true;
				EnhancedInputComponent->BindAction(PlaceAction, ETriggerEvent::Started, this, &UBuildingComponent::PlacePiece);
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

	const auto SpawnedPiece = GetWorld()->SpawnActor<ABuildingPieceBase>(CurrentBuildPiece, PreviewTransform);
	SpawnedPiece->Placed();
}

FTransform UBuildingComponent::CheckForSnappingSockets(const FHitResult& Hit)
{
	if (!Hit.GetActor()) return FTransform();
	if (!Hit.GetActor()->Implements<UBuildInterface>()) return FTransform(FRotator::ZeroRotator, Hit.ImpactPoint);

	const ABuildingPieceBase* Piece = Cast<ABuildingPieceBase>(Hit.GetActor());

	const UBoxComponent* Check = Cast<UBoxComponent>(Hit.GetComponent());
	PlacementBlocked = Check == nullptr;
	PreviewMesh->ToggleIncorrectMaterial(PlacementBlocked);

	TArray<UBoxComponent*> Sockets = Piece->GetSnappingSockets();
	for (UBoxComponent* Socket : Sockets)
	{
		if(Cast<UPrimitiveComponent>(Socket) != Hit.GetComponent()) continue;
		return Socket->GetComponentTransform();
	}

	return FTransform(FRotator::ZeroRotator, Hit.ImpactPoint);
}
