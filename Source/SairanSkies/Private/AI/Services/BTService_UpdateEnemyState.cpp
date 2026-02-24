// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Services/BTService_UpdateEnemyState.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "AI/GroupCombatManager.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

UBTService_UpdateEnemyState::UBTService_UpdateEnemyState()
{
	NodeName = TEXT("Update Enemy State");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
}

void UBTService_UpdateEnemyState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AEnemyAIController* AIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return;
	}

	AEnemyBase* Enemy = AIController->GetControlledEnemy();
	if (!Enemy)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return;
	}

	// Update target info
	BlackboardComp->SetValueAsObject(AEnemyBase::BB_TargetActor, Enemy->GetCurrentTarget());
	
	// Only update TargetLocation if we have a target (don't overwrite patrol points!)
	if (Enemy->GetCurrentTarget())
	{
		BlackboardComp->SetValueAsVector(AEnemyBase::BB_TargetLocation, Enemy->GetLastKnownTargetLocation());
	}
	
	// Update state values
	BlackboardComp->SetValueAsInt(AEnemyBase::BB_EnemyState, static_cast<int32>(Enemy->GetEnemyState()));
	BlackboardComp->SetValueAsBool(AEnemyBase::BB_CanSeeTarget, Enemy->CanSeeTarget());
	BlackboardComp->SetValueAsFloat(AEnemyBase::BB_DistanceToTarget, Enemy->GetDistanceToTarget());

	// Update proximity priorities in GroupCombatManager (if target exists)
	if (UGroupCombatManager* CombatManager = GetWorld()->GetSubsystem<UGroupCombatManager>())
	{
		// Update MaxInnerCircleEnemies from config
		CombatManager->MaxInnerCircleEnemies = Enemy->CombatConfig.MaxSimultaneousAttackers;
	}

	// CanAttack = enemy is in inner circle (or has space) AND attack cooldown is ready
	bool bCanAttack = Enemy->CanAttackNow();
	if (UGroupCombatManager* CombatManager = GetWorld()->GetSubsystem<UGroupCombatManager>())
	{
		bCanAttack = bCanAttack && CombatManager->IsInInnerCircle(Enemy);
	}
	BlackboardComp->SetValueAsBool(AEnemyBase::BB_CanAttack, bCanAttack);
}

FString UBTService_UpdateEnemyState::GetStaticDescription() const
{
	return TEXT("Updates enemy state values in Blackboard");
}
