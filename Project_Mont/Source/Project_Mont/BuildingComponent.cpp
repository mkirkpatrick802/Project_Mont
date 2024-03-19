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

	SetupInput();
}

void UBuildingComponent::SetupInput()
{
	if(!Character) return;

	if(const APlayerController* Controller = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(BuildingMappingContext, 1);
		}

		if(UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(Controller->InputComponent))
		{
			EnhancedInputComponent->BindAction(PlaceAction, ETriggerEvent::Started, this, &UBuildingComponent::PlacePiece);
			EnhancedInputComponent->BindAction(BuildModeAction, ETriggerEvent::Started, this, &UBuildingComponent::ToggleBuildMode);
		}
	}

	PlaceAction->bConsumeInput = false;
}

void UBuildingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(const auto TPPCharacter = Cast<ATPPCharacter>(Character))
	{
		FHitResult HitResult;
		CrosshairUtility::TraceUnderCrosshairs(TPPCharacter, HitResult, true, 100, MaxBuildDistance);

		if (HitResult.GetActor())
		{
			PreviewLocation = HitResult.ImpactPoint;
		}
	}
}

void UBuildingComponent::ToggleBuildMode(const FInputActionValue& InputActionValue)
{
	IsBuilding = !IsBuilding;
	PlaceAction->bConsumeInput = IsBuilding;
}

void UBuildingComponent::PlacePiece(const FInputActionValue& InputActionValue)
{
	if(!IsBuilding) return;
	if(PreviewLocation == FVector::Zero()) return;

	const auto SpawnedPiece = GetWorld()->SpawnActor<ABuildingPieceBase>(CurrentBuildPiece, PreviewLocation, FRotator::ZeroRotator);
	SpawnedPiece->Placed();
}
