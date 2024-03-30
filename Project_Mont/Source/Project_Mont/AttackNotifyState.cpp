#include "AttackNotifyState.h"

#include "EnemyCharacterBase.h"

void UAttackNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, 
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if(MeshComp && MeshComp->GetOwner())
	{
		if(AEnemyCharacterBase* Character = Cast<AEnemyCharacterBase>(MeshComp->GetOwner()))
		{
			Character->MeleeAttackCheck();
		}
	}
}

void UAttackNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (AEnemyCharacterBase* Character = Cast<AEnemyCharacterBase>(MeshComp->GetOwner()))
		{
			Character->MeleeAttackEnd();
		}
	}
}
