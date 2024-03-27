#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Project_MontGameModeBase.generated.h"

class AVoxelActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEggStateChanged, bool, NewEggState);

UCLASS()
class PROJECT_MONT_API AProject_MontGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

private:

	void ActivateEgg() const;

	void DeactivateEgg() const;

public:

	UPROPERTY(BlueprintAssignable)
	FEggStateChanged EggStateChanged;
};
