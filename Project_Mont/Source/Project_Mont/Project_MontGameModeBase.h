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

	void ActivateEgg();
	void DeactivateEgg();

public:

	UPROPERTY(BlueprintAssignable)
	FEggStateChanged EggStateChanged;

	UPROPERTY(EditAnywhere, Category = Egg)
	float EggActivationMinimum = 120;

	UPROPERTY(EditAnywhere, Category = Egg)
	float EggActivationMaximum = 240;

	UPROPERTY(EditAnywhere, Category = Egg)
	float EggDeactivationMinimum = 60;

	UPROPERTY(EditAnywhere, Category = Egg)
	float EggDeactivationMaximum = 90;

private:

	bool CurrentEggState = false;
};
