// SairanSkies - Zona de Oleadas

#include "Core/WaveZone.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Enemies/EnemyBase.h"
#include "GameFramework/Character.h"
#include "Sound/SoundBase.h"

// ─── AWaveZone ────────────────────────────────────────────────────────────────

AWaveZone::AWaveZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneTrigger"));
	ZoneTrigger->InitBoxExtent(FVector(500.f, 500.f, 200.f));
	ZoneTrigger->SetCollisionProfileName(TEXT("OverlapAll"));
	RootComponent = ZoneTrigger;
}

void AWaveZone::BeginPlay()
{
	Super::BeginPlay();
	ZoneTrigger->OnComponentBeginOverlap.AddDynamic(this, &AWaveZone::OnBeginOverlap);
	ZoneTrigger->OnComponentEndOverlap.AddDynamic(this, &AWaveZone::OnEndOverlap);
}

// ── Overlap ───────────────────────────────────────────────────────────────────

void AWaveZone::OnBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character || !Cast<APlayerController>(Character->GetController())) return;

	bPlayerInZone = true;
	UE_LOG(LogTemp, Log, TEXT("WaveZone: Jugador entró"));

	if (!bAllWavesCleared && CurrentWave == 0)
	{
		StartWave(1);
	}
}

void AWaveZone::OnEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character || !Cast<APlayerController>(Character->GetController())) return;

	bPlayerInZone = false;
	UE_LOG(LogTemp, Warning, TEXT("WaveZone: Jugador salió — reiniciando"));
	ResetZone();
}

// ── Lógica de oleadas ─────────────────────────────────────────────────────────

void AWaveZone::StartWave(int32 WaveIndex)
{
	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("WaveZone [%s]: EnemyClass no asignada. Configúralo en el panel Details."),
			*GetName());
		return;
	}

	// Comprobar límite de oleadas
	if (MaxWaves > 0 && WaveIndex > MaxWaves)
	{
		bAllWavesCleared = true;
		UE_LOG(LogTemp, Warning, TEXT("WaveZone: ¡Todas las oleadas completadas! (%d)"), MaxWaves);
		if (AllWavesClearedSound)
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), AllWavesClearedSound, GetActorLocation());
		return;
	}

	CurrentWave = WaveIndex;

	// 2^(WaveIndex-1) → oleada 1 = 1×, oleada 2 = 2×, oleada 3 = 4×…
	const int32 TotalEnemies = BaseEnemyCount
		* FMath::RoundToInt(FMath::Pow(2.0f, static_cast<float>(WaveIndex - 1)));

	EnemiesAliveCount   = TotalEnemies;
	EnemiesToSpawnQueue = TotalEnemies;

	UE_LOG(LogTemp, Warning, TEXT("WaveZone: ── Oleada %d ── %d enemigos"), CurrentWave, TotalEnemies);

	if (WaveStartSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), WaveStartSound, GetActorLocation());

	SpawnNextEnemy();
}

void AWaveZone::SpawnNextEnemy()
{
	if (!EnemyClass || EnemiesToSpawnQueue <= 0) return;

	FVector SpawnLoc;
	if (!FindSpawnLocation(SpawnLoc))
	{
		// Fallback: centro + offset pequeño
		SpawnLoc = GetActorLocation() + FVector(
			FMath::RandRange(-100.f, 100.f),
			FMath::RandRange(-100.f, 100.f), 0.f);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AEnemyBase* Enemy = GetWorld()->SpawnActor<AEnemyBase>(EnemyClass, SpawnLoc,
		FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f), Params);

	if (Enemy)
	{
		ActiveEnemies.Add(Enemy);
		Enemy->OnEnemyDeath.AddDynamic(this, &AWaveZone::OnEnemyDied);
	}
	else
	{
		// Spawn fallido: decrementar para no bloquear el avance de oleada
		EnemiesAliveCount = FMath::Max(0, EnemiesAliveCount - 1);
		UE_LOG(LogTemp, Warning, TEXT("WaveZone: Fallo al spawnear enemigo (posición bloqueada)"));
	}

	EnemiesToSpawnQueue--;

	if (EnemiesToSpawnQueue > 0)
	{
		GetWorldTimerManager().SetTimer(
			SpawnStaggerTimer, this, &AWaveZone::SpawnNextEnemy, SpawnStagger, false);
	}
}

void AWaveZone::OnEnemyDied(AController* /*InstigatorController*/)
{
	EnemiesAliveCount = FMath::Max(0, EnemiesAliveCount - 1);

	UE_LOG(LogTemp, Log, TEXT("WaveZone: Enemigo muerto (%d vivos, %d en cola)"),
		EnemiesAliveCount, EnemiesToSpawnQueue);

	// Avanzar oleada cuando: todos spawneados Y todos muertos Y jugador dentro
	if (EnemiesAliveCount <= 0 && EnemiesToSpawnQueue <= 0 && bPlayerInZone && !bAllWavesCleared)
	{
		UE_LOG(LogTemp, Warning, TEXT("WaveZone: ¡Oleada %d completada!"), CurrentWave);
		ActiveEnemies.Empty();
		StartWave(CurrentWave + 1);
	}
}

// ── Reinicio ──────────────────────────────────────────────────────────────────

void AWaveZone::ResetZone()
{
	GetWorldTimerManager().ClearTimer(SpawnStaggerTimer);

	for (AEnemyBase* Enemy : ActiveEnemies)
	{
		if (IsValid(Enemy))
		{
			Enemy->OnEnemyDeath.RemoveDynamic(this, &AWaveZone::OnEnemyDied);
			Enemy->Destroy();
		}
	}
	ActiveEnemies.Empty();

	CurrentWave         = 0;
	EnemiesAliveCount   = 0;
	EnemiesToSpawnQueue = 0;
	bAllWavesCleared    = false;

	UE_LOG(LogTemp, Log, TEXT("WaveZone: Zona reiniciada a oleada 0"));
}

// ── Spawn location ────────────────────────────────────────────────────────────

bool AWaveZone::FindSpawnLocation(FVector& OutLocation) const
{
	const FVector ZoneCenter = GetActorLocation();

	for (int32 Attempt = 0; Attempt < 6; ++Attempt)
	{
		const FVector RandomOffset(
			FMath::RandRange(-SpawnRadius, SpawnRadius),
			FMath::RandRange(-SpawnRadius, SpawnRadius),
			SpawnTraceHeight);

		const FVector TraceStart = ZoneCenter + RandomOffset;
		const FVector TraceEnd   = TraceStart - FVector(0.f, 0.f, SpawnTraceHeight * 2.5f);

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd,
			ECC_WorldStatic, Params))
		{
			OutLocation = Hit.ImpactPoint + FVector(0.f, 0.f, 95.f);
			return true;
		}
	}
	return false;
}
