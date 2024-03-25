#pragma once

#include "CoreMinimal.h"
#include "BuildingPieceBase.h"
#include "SocketInterface.h"
#include "Components/BoxComponent.h"
#include "SocketComponent.generated.h"

UCLASS()
class PROJECT_MONT_API USocketComponent : public UBoxComponent, public ISocketInterface
{
	GENERATED_BODY()
};
