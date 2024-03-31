#include "Health.h"

Health::Health()
{
	MaxHealth = 100;
	CurrentHealth = MaxHealth;
}

Health::Health(float SetMaxHealth)
{
	MaxHealth = SetMaxHealth;
	CurrentHealth = MaxHealth;
}