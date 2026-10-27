#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXSpeedBuffItem.generated.h"

class USceneComponent;
class UPrimitiveComponent;
class USphereComponent;
class UParticleSystem;
class UStaticMeshComponent;
class USoundBase;

UCLASS()
class CX_API ACXSpeedBuffItem : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ACXSpeedBuffItem();

protected:
	
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPickupEffects();

protected:	
	
	virtual void Tick(float DeltaTime) override;


protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
	TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
	TObjectPtr<USphereComponent> OverlapSphere;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
	TObjectPtr<UStaticMeshComponent> ItemMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coin")
	TObjectPtr<UParticleSystem> PickupParticle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coin")
	TObjectPtr<USoundBase> PickupSound;

};
