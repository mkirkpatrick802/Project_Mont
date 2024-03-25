#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BuildInterface.generated.h"

class USocketComponent;

UINTERFACE(MinimalAPI)
class UBuildInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECT_MONT_API IBuildInterface
{
	GENERATED_BODY()

public:

	virtual TArray<USocketComponent*> GetSnappingSockets() const = 0;

};
