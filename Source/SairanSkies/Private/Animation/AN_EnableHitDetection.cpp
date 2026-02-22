// SairanSkies - Animation Notify Implementation

#include "Animation/AN_EnableHitDetection.h"
#include "Character/SairanCharacter.h"
#include "Combat/CombatComponent.h"

void UAN_EnableHitDetection::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	ASairanCharacter* Character = Cast<ASairanCharacter>(MeshComp->GetOwner());
	if (Character && Character->CombatComponent)
	{
		Character->CombatComponent->EnableHitDetection();
	}
}

void UAN_EnableHitDetection::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	ASairanCharacter* Character = Cast<ASairanCharacter>(MeshComp->GetOwner());
	if (Character && Character->CombatComponent)
	{
		Character->CombatComponent->DisableHitDetection();
	}
}
