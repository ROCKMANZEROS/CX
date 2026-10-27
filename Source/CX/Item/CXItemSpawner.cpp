#include "Item/CXItemSpawner.h"
#include "Components/BillboardComponent.h"


ACXItemSpawner::ACXItemSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	SetRootComponent(BillboardComponent);
}

void ACXItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnItem();
	}
	
}

void ACXItemSpawner::SpawnItem()
{
	if (!HasAuthority() || IsValid(ItemClass) == false)
	{
		return;
	}

	UWorld* World = GetWorld();
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	SpawnedItem = World->SpawnActorDeferred<AActor>(ItemClass, GetActorTransform(), this,
		nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	SpawnedItem->OnDestroyed.AddDynamic(this, &ACXItemSpawner::OnItemDestroyed);
	SpawnedItem->FinishSpawning(GetActorTransform());
}

void ACXItemSpawner::OnItemDestroyed(AActor* DestroyedActor)
{
	if (!HasAuthority() || IsValid(ItemClass) == false)
	{
		return;
	}

	SpawnedItem = nullptr;

	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ACXItemSpawner::SpawnItem, SpawnInterval, false);
}


