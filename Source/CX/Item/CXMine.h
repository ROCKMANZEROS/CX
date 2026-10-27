#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXMine.generated.h"

class USceneComponent;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;
class USoundBase;

UCLASS()
class CX_API ACXMine : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ACXMine();

protected:
	
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplodeEffects();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<UStaticMeshComponent> MineMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<UParticleSystem> ExplodeParticle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<USoundBase> ExplodeSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mine")
	float ThrottleBreakDuration = 2.0f;

};
