#pragma once

#include "CoreMinimal.h"

class PROJECT_MONT_API Health
{
public:

    Health();
	Health(float SetMaxHealth);

    // Overloading += operator
    float operator+=(const float& Right) {
        CurrentHealth += Right;
        return CurrentHealth;
    }

    // Overloading -= operator
    float operator-=(const float& Right) {
        CurrentHealth -= Right;
        return CurrentHealth;
    }

    // Overloading == operator
    bool operator==(const float& Right) const {
        return CurrentHealth == Right;
    }

private:

	float MaxHealth;
	float CurrentHealth;

public:

    FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }
    FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

};
