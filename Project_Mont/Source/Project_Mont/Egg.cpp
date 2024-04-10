#include "Egg.h"

AEgg::AEgg()
{
}

void AEgg::BeginPlay()
{
	Super::BeginPlay();

	EggHealth = MaxHealth;
}

void AEgg::Hit(const float Damage)
{
	EggHealth -= Damage;

	if(EggHealth.GetCurrentHealth() <= 0)
	{
		Destroy();
	}
}
