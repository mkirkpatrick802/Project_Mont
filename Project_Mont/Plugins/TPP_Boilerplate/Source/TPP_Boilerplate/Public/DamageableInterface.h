#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DamageableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UDamageableInterface : public UInterface
{
	GENERATED_BODY()
};

class TPP_BOILERPLATE_API IDamageableInterface
{
	GENERATED_BODY()

public:

	virtual void Hit(const float Damage) = 0;

};
