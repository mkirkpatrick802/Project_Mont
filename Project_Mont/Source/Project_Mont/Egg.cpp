#include "Egg.h"

AEgg::AEgg()
{
	EggHealth = 100;
}

void AEgg::Hit(const float Damage)
{
	EggHealth -= Damage;

	if(EggHealth.GetCurrentHealth() <= 0)
	{
		Destroy();
	}
}
