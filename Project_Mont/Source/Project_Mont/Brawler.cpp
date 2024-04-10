#include "Brawler.h"

#include "EnemyControllerBase.h"
#include "GameFramework/CharacterMovementComponent.h"

void ABrawler::BeginPlay()
{
	Super::BeginPlay();

	CurrentState = EBrawlerEnemyState::BES_Wander;
	GetCharacterMovement()->MaxWalkSpeed = WanderMaxWalkSpeed;


	EnemyController->CurrentTargetChangedDelegate.AddDynamic(this, &ABrawler::TargetStateChanged);
}

void ABrawler::TargetStateChanged(const ETargetState NewState)
{
	switch(NewState)
	{
	case ETargetState::TS_Player:

		if (IsEggActive)
		{
			SetCurrentState(EBrawlerEnemyState::BES_Charge);
		}
		else
		{
			SetCurrentState(EBrawlerEnemyState::BES_Chase);
		}

		break;
	case ETargetState::TS_Egg:

		SetCurrentState(EBrawlerEnemyState::BES_Charge);

		break;
	case ETargetState::TS_Update:

		SetCurrentState(EBrawlerEnemyState::BES_Wander);

		break;
	}
}

void ABrawler::SetCurrentState(const EBrawlerEnemyState NewState)
{
	CurrentState = NewState;

	switch (CurrentState)
	{
	case EBrawlerEnemyState::BES_Wander:

		GetCharacterMovement()->MaxWalkSpeed = WanderMaxWalkSpeed;

		break;
	case EBrawlerEnemyState::BES_Chase:

		GetCharacterMovement()->MaxWalkSpeed = ChaseMaxWalkSpeed;

		break;
	case EBrawlerEnemyState::BES_Charge:

		GetCharacterMovement()->MaxWalkSpeed = ChargeMaxWalkSpeed;

		break;
	}
}