#pragma once

#include "CoreMinimal.h"
#include "Health.h"
#include "PickupObject.h"
#include "Egg.generated.h"

UCLASS()
class PROJECT_MONT_API AEgg : public APickupObject
{
	GENERATED_BODY()

public:

	Health EggHealth;

};
