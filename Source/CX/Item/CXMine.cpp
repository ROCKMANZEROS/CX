#include "Item/CXMine.h"
#include "CXMine.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CXPawn.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

ACXMine::ACXMine()
{
 	
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

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

	MineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MineMesh"));
	MineMesh->SetupAttachment(SceneRoot);
	MineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


void ACXMine::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapSphereBeginOverlap);
	}
}

void ACXMine::OnOverlapSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority() == false)
	{
		return;
	}

	ACXPawn* VehiclePawn = Cast<ACXPawn>(OtherActor);
	if (IsValid(VehiclePawn) == false)
	{
		return;
	}

	// 가속 페달 고장 적용
	VehiclePawn->ApplyThrottleBreak(ThrottleBreakDuration);

	MulticastExplodeEffects();

	Destroy();
}

void ACXMine::MulticastExplodeEffects_Implementation()
{
	UWorld* World = GetWorld();

	if (IsValid(ExplodeParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(World, ExplodeParticle, GetActorLocation());
	}

	if (IsValid(ExplodeSound))
	{
		UGameplayStatics::PlaySoundAtLocation(World, ExplodeSound, GetActorLocation());
	}
}


