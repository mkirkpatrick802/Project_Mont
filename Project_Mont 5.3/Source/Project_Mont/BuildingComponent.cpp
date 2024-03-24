#include "BuildingComponent.h"

#include "BuildingPieceBase.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "CrosshairUtility.h"
#include "TPPCharacter.h"

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
		CrosshairUtility::TraceUnderCrosshairs(TPPCharacter, HitResult, OffsetLength, MaxBuildDistance + (OffsetLength/2), ECC_Visibility, QueryParams);

		if (HitResult.GetActor())
		{
			PreviewLocation = HitResult.ImpactPoint;
			PreviewMesh->SetActorLocation(PreviewLocation);
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
	ToggleBuildMode();
}

void UBuildingComponent::ToggleBuildModeInput(const FInputActionValue& InputActionValue)
{
	if (!CurrentBuildPiece) return;
	ToggleBuildMode();
}

void UBuildingComponent::ToggleBuildMode()
{
	IsBuilding = !IsBuilding;

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
	if(!IsBuilding || !CurrentBuildPiece) return;
	if(PreviewLocation == FVector::Zero()) return;

	const auto SpawnedPiece = GetWorld()->SpawnActor<ABuildingPieceBase>(CurrentBuildPiece, PreviewLocation, FRotator::ZeroRotator);
	SpawnedPiece->Placed();
}
