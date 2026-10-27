#include "Item/CXSpeedBuffItem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CXPawn.h"
#include "PlayerState/CXPlayerState.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

ACXSpeedBuffItem::ACXSpeedBuffItem()
{ 	
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	NetUpdateFrequency = 1.0f;
	MinNetUpdateFrequency = 1.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(SceneRoot);
	OverlapSphere->SetSphereRadius(100.f);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Vehicle, ECollisionResponse::ECR_Overlap);
	OverlapSphere->SetGenerateOverlapEvents(true);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(SceneRoot);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);




}


void ACXSpeedBuffItem::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapSphereBeginOverlap);
	}
	
}

void ACXSpeedBuffItem::OnOverlapSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	ACXPawn* VehiclePawn = Cast<ACXPawn>(OtherActor);
	if (IsValid(VehiclePawn) == false)
	{
		return;
	}

	ACXPlayerState* PlayerState = VehiclePawn->GetPlayerState<ACXPlayerState>();
	if (IsValid(PlayerState) == false)
	{
		return;
	}

	PlayerState->AddBonusMaxSpeed(10.0f);

	MulticastPickupEffects();

	Destroy();
}

void ACXSpeedBuffItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACXSpeedBuffItem::MulticastPickupEffects_Implementation()
{
	UWorld* World = GetWorld();

	if (IsValid(PickupParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(World, PickupParticle, GetActorLocation());
	}

	if (IsValid(PickupSound))
	{
		UGameplayStatics::PlaySoundAtLocation(World, PickupSound, GetActorLocation());
	}
}
