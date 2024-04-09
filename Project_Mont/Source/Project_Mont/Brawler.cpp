#include "Brawler.h"

#include "GameFramework/CharacterMovementComponent.h"

void ABrawler::BeginPlay()
{
	Super::BeginPlay();

	CurrentState = EBrawlerEnemyState::BES_Wander;
	GetCharacterMovement()->MaxWalkSpeed = WanderMaxWalkSpeed;
}

void ABrawler::EggStateChanged(bool IsActive)
{
	Super::EggStateChanged(IsActive);

	SetCurrentState(IsActive ? EBrawlerEnemyState::BES_Charge : EBrawlerEnemyState::BES_Wander);
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
