#include "MeleeWeapon.h"

void AMeleeWeapon::Melee()
{
	Super::Melee();

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(3, 15.0f, FColor::Blue, FString::Printf(TEXT("Melee Attack")));
}
