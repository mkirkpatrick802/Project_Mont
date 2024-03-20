// Fill out your copyright notice in the Description page of Project Settings.


#include "CrosshairUtility.h"

#include "InteractInterface.h"
#include "InteractWithCrosshairsInterface.h"
#include "TPPHUD.h"
#include "Kismet/GameplayStatics.h"
#include "TPPCharacter.h"
#include "GameFramework/SpringArmComponent.h"

CrosshairUtility::CrosshairUtility()
{
}

CrosshairUtility::~CrosshairUtility()
{
}

void CrosshairUtility::TraceUnderCrosshairs(const ATPPCharacter* Character, FHitResult& TraceHitResult, float OffsetLength, float TraceLength, ECollisionChannel TraceChannel, FCollisionQueryParams CollisionQueryParams)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	const FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	const bool ScreenToWorldWorked = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(Character, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (ScreenToWorldWorked)
	{
		FVector Start = CrosshairWorldPosition;

		if (Character && OffsetLength > 0)
		{
			const float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + (Character->GetCameraBoom()->TargetArmLength - OffsetLength));
		}

		const FVector End = Start + CrosshairWorldDirection * TraceLength;

		Character->GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, TraceChannel, CollisionQueryParams);

		if (!TraceHitResult.bBlockingHit)
			TraceHitResult.ImpactPoint = End;
	}
}
