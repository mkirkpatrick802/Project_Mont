#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BuildInterface.generated.h"

class UBoxComponent;

UINTERFACE(MinimalAPI)
class UBuildInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECT_MONT_API IBuildInterface
{
	GENERATED_BODY()

public:

	virtual TArray<UBoxComponent*> GetSnappingSockets() const = 0;

};
