#include "EnemySpawnerComponent.h"

#include "Project_MontGameModeBase.h"
#include "Kismet/GameplayStatics.h"

UEnemySpawnerComponent::UEnemySpawnerComponent(): Debugging(false), MinRadius(0), MaxRadius(0), IslandZ(0)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemySpawnerComponent::BeginPlay()
{
	Super::BeginPlay();

	IslandZ = GetOwner()->GetActorLocation().Z;

	AProject_MontGameModeBase* GameMode = Cast<AProject_MontGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	GameMode->EggStateChanged.AddDynamic(this, &UEnemySpawnerComponent::EggStateChanged);
}

void UEnemySpawnerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(!Debugging || !GetWorld()) return;

	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = Start + FVector(0, 0, 1000);
	DrawDebugCylinder(GetWorld(), Start, End, MinRadius, 12, FColor::Blue);
	DrawDebugCylinder(GetWorld(), Start, End, MaxRadius, 12, FColor::Red);
	DrawDebugSphere(GetWorld(), FVector(GetOwner()->GetActorLocation().X, GetOwner()->GetActorLocation().Y, GetOwner()->GetActorLocation().Z + SpawningCheckZOffset), 
	100, 16, FColor::Yellow, false, -1, 0, 5);
}

void UEnemySpawnerComponent::SpawnWave(const int EnemyCount)
{
	for (int i = 0; i < EnemyCount; i++)
	{
		const FVector SpawnLocation = FindSpawnLocation();
		if(SpawnLocation == FVector::ZeroVector) return;

		SpawnEnemy(SpawnLocation + FVector(0, 0, 150), ChaserEnemy);
	}
}

void UEnemySpawnerComponent::SpawnEnemy(const FVector& Location, TSubclassOf<AEnemyCharacterBase> ToSpawn)
{
	if (AEnemyCharacterBase* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyCharacterBase>(ChaserEnemy, Location, FRotator::ZeroRotator))
	{
		SpawnedEnemy->SpawnDefaultController();
		SpawnedEnemy->HasDiedDelegate.AddDynamic(this, &UEnemySpawnerComponent::EnemyDied);
		SpawnedEnemy->Spawner = this;

		SpawnedEnemies.AddUnique(SpawnedEnemy);
	}
}

FVector UEnemySpawnerComponent::FindSpawnLocation()
{
	const float Theta = FMath::FRandRange(0.f, 2 * PI);
	const float Radius = FMath::FRandRange(MinRadius, MaxRadius);

	const float X = Radius * FMath::Cos(Theta);
	const float Y = Radius * FMath::Sin(Theta);
	const float Z = IslandZ + SpawningCheckZOffset;

	const FVector Start = FVector(X, Y, Z);
	const FVector End = FVector(X, Y, IslandZ - SpawningCheckZOffset);

	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);

	return HitResult.ImpactPoint;
}

void UEnemySpawnerComponent::EnemyDied(AEnemyCharacterBase* DeadEnemy)
{
	SpawnedEnemies.Remove(DeadEnemy);
	DeadEnemy->Destroy();
}

void UEnemySpawnerComponent::EggStateChanged(bool NewEggState)
{
	for (const auto SpawnedEnemy : SpawnedEnemies)
	{
		SpawnedEnemy->EggStateChanged(NewEggState);
	}
}