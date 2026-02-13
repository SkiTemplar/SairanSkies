// SairanSkies - Base Enemy Implementation

#include "Enemies/EnemyBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Setup capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Add Enemy tag for targeting system
	Tags.Add(FName("Enemy"));
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentHealth = MaxHealth;
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.0f;

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	CurrentHealth -= ActualDamage;
	
	// Broadcast damage event
	OnDamaged.Broadcast(ActualDamage);

	UE_LOG(LogTemp, Log, TEXT("%s took %.1f damage. Health: %.1f/%.1f"), 
		*GetName(), ActualDamage, CurrentHealth, MaxHealth);

	// Check for death
	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return ActualDamage;
}

void AEnemyBase::StartAttackWindup()
{
	bIsInAttackWindup = true;

	// End windup after duration
	GetWorld()->GetTimerManager().SetTimer(WindupTimer, this, &AEnemyBase::EndAttackWindup, AttackWindupDuration, false);
}

void AEnemyBase::EndAttackWindup()
{
	bIsInAttackWindup = false;
}

void AEnemyBase::Die()
{
	if (bIsDead) return;

	bIsDead = true;
	CurrentHealth = 0.0f;

	// Broadcast death event
	OnDeath.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("%s has died!"), *GetName());

	// Disable collision and movement
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();

	// Optional: Destroy after delay (or play death animation)
	SetLifeSpan(3.0f);
}

void AEnemyBase::ResetHealth()
{
	CurrentHealth = MaxHealth;
	bIsDead = false;
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}
