#include "EnemySpawnerComponent.h"

UEnemySpawnerComponent::UEnemySpawnerComponent(): Debugging(false), MinRadius(0), MaxRadius(0), IslandZ(0)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemySpawnerComponent::BeginPlay()
{
	Super::BeginPlay();

	IslandZ = GetOwner()->GetActorLocation().Z;
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
}

void UEnemySpawnerComponent::SpawnWave(const int EnemyCount)
{
	for (int i = 0; i < EnemyCount; i++)
	{
		const FVector SpawnLocation = FindSpawnLocation() + FVector(0, 0, 150);
		SpawnEnemy(SpawnLocation, ChaserEnemy);
	}
}

void UEnemySpawnerComponent::SpawnEnemy(const FVector& Location, TSubclassOf<AEnemyCharacterBase> ToSpawn) const
{
	if (AEnemyCharacterBase* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyCharacterBase>(ChaserEnemy, Location, FRotator::ZeroRotator))
	{
		SpawnedEnemy->SpawnDefaultController();
	}
}

FVector UEnemySpawnerComponent::FindSpawnLocation()
{
	const float Theta = FMath::FRandRange(0.f, 2 * PI);
	const float Radius = FMath::FRandRange(MinRadius, MaxRadius);

	const float X = Radius * FMath::Cos(Theta);
	const float Y = Radius * FMath::Sin(Theta);
	const float Z = IslandZ + 3000;

	const FVector Start = FVector(X, Y, Z);
	const FVector End = FVector(X, Y, IslandZ - 1000);

	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);

	return HitResult.ImpactPoint;
}