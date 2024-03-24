#include "MeleeWeapon.h"

void AMeleeWeapon::Attack(const FVector& HitTarget)
{
	Super::Attack();

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("Axe Attack")));
}
