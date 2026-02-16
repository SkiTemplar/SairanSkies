// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Services/BTService_UpdateEnemyState.h"
#include "Enemies/EnemyBase.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

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

	BlackboardComp->SetValueAsObject(AEnemyBase::BB_TargetActor, Enemy->GetCurrentTarget());
	
	// Only update TargetLocation if we have a target (don't overwrite patrol points!)
	if (Enemy->GetCurrentTarget())
	{
		BlackboardComp->SetValueAsVector(AEnemyBase::BB_TargetLocation, Enemy->GetLastKnownTargetLocation());
	}
	
	BlackboardComp->SetValueAsInt(AEnemyBase::BB_EnemyState, static_cast<int32>(Enemy->GetEnemyState()));
	BlackboardComp->SetValueAsBool(AEnemyBase::BB_CanSeeTarget, Enemy->CanSeeTarget());
	BlackboardComp->SetValueAsBool(AEnemyBase::BB_ShouldTaunt, Enemy->ShouldTaunt());
	BlackboardComp->SetValueAsInt(AEnemyBase::BB_NearbyAllies, Enemy->GetNearbyAlliesCount());
	BlackboardComp->SetValueAsFloat(AEnemyBase::BB_DistanceToTarget, Enemy->GetDistanceToTarget());
	
	// AAA-style awareness system values
	BlackboardComp->SetValueAsFloat(AEnemyBase::BB_SuspicionLevel, Enemy->GetSuspicionLevel());
	BlackboardComp->SetValueAsBool(AEnemyBase::BB_IsAlerted, Enemy->IsAlerted());
	BlackboardComp->SetValueAsBool(AEnemyBase::BB_IsInPause, Enemy->IsInRandomPause());
}

FString UBTService_UpdateEnemyState::GetStaticDescription() const
{
	return TEXT("Updates enemy state values in Blackboard");
}
