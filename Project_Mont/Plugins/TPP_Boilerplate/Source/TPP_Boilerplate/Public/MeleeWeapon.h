#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "MeleeWeapon.generated.h"

UCLASS()
class TPP_BOILERPLATE_API AMeleeWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Melee() override;

};
