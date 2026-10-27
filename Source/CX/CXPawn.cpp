// Copyright Epic Games, Inc. All Rights Reserved.

#include "CXPawn.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "Item/CXMine.h"
#include "PlayerState/CXPlayerState.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameState/CXGameState.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/DamageEvents.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);

		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ACXPawn::InputAttack);

		if (IsValid(PlaceMineAction) == true)
		{
			EnhancedInputComponent->BindAction(PlaceMineAction, ETriggerEvent::Started, this, &ACXPawn::InputPlaceMine);
		}
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));

	// 게임 상태에 따른 핸드브레이크 잠금
	if (IsLocallyControlled() == true || HasAuthority() == true)
	{
		if (IsGamePlaying() == false)
		{
			ChaosVehicleMovement->SetHandbrakeInput(true);
			bGameStateHandbrakeLocked = true;
		}
		else if (bGameStateHandbrakeLocked == true)
		{
			ChaosVehicleMovement->SetHandbrakeInput(false);
			bGameStateHandbrakeLocked = false;
		}
	}

	// 속도 패널티 회복 (서버에서만)
	if (HasAuthority() == true)
	{
		ACXPlayerState* CXPlayerState = GetPlayerState<ACXPlayerState>();
		if (IsValid(CXPlayerState) == true)
		{
			CXPlayerState->RecoverBonusMaxSpeedPenalty(PenaltyRecoverPerSecond * Delta);
		}
	}
}

float ACXPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	ACXPawn* Attacker = Cast<ACXPawn>(DamageCauser);

	if (ActualDamage <= 0 || DamageCauser == this || HasAuthority() == false || Attacker == nullptr)
	{
		return 0.f;
	}

	// 맞은 사람
	ACXPlayerState* VictimPlayerState = GetPlayerState<ACXPlayerState>();
	// 때린 사람
	ACXPlayerState* AttackerPlayerState = Attacker->GetPlayerState<ACXPlayerState>();

	if (IsValid(VictimPlayerState) == false || IsValid(AttackerPlayerState) == false)
	{
		return 0.f;
	}

	// 포인트 데미지면 맞은 Bone이 바퀴인지 확인
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID) == true)
	{
		const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
		const FString BoneName = PointDamageEvent->HitInfo.BoneName.ToString();
		UE_LOG(LogTemplateVehicle, Warning, TEXT("Hit bone: %s"), *BoneName);

		if (BoneName.Contains(TEXT("Wheel")) == true)
		{
			VictimPlayerState->AddBonusMaxSpeed(-WheelHitSpeedPenalty);
			UE_LOG(LogTemplateVehicle, Warning, TEXT("Wheel hit! %s -> %s"), *GetNameSafe(VictimPlayerState), *VictimPlayerState->GetPlayerStateInfo());
		}
	}

	const int32 TransferCoinAmount = (int32)ActualDamage;

	if (VictimPlayerState->TryRemoveCoins((int32)TransferCoinAmount))
	{
		UE_LOG(LogTemplateVehicle, Warning, TEXT("%s transferred %d coins to %s"), *GetNameSafe(VictimPlayerState), TransferCoinAmount, *GetNameSafe(AttackerPlayerState));
		AttackerPlayerState->AddCoins(TransferCoinAmount);
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Warning, TEXT("%s has no coins to transfer %s"), *GetNameSafe(VictimPlayerState), *GetNameSafe(AttackerPlayerState));
	}

	return ActualDamage;
}

bool ACXPawn::IsGamePlaying() const
{
	const UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return false;
	}

	const ACXGameState* CXGameState = World->GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		return false;
	}

	return CXGameState->GetCurrentGameState() == EGameState::Playing;
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
	// get the input magnitude for steering
	float SteeringValue = Value.Get<float>();

	// Playing 상태가 아니면 조향 입력을 0으로
	if (IsGamePlaying() == false)
	{
		SteeringValue = 0.0f;
	}

	// add the input
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	// get the input magnitude for the throttle
	float ThrottleValue = Value.Get<float>();

	// 가속 페달이 고장났거나 Playing 상태가 아니면 입력을 0으로
	if (bThrottleBroken == true || IsGamePlaying() == false)
	{
		ThrottleValue = 0.0f;
	}

	// add the input
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	// get the input magnitude for the brakes
	float BreakValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetBrakeInput(BreakValue);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(false);

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	// get the flat angle value for the input 
	float LookValue = Value.Get<float>();

	// add the input
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);

	UE_LOG(LogTemplateVehicle, Error, TEXT("Reset Vehicle"));
}

void ACXPawn::InputAttack(const FInputActionValue& Value)
{
	if (IsGamePlaying() == false)
	{
		return;
	}

	CTSAttack();
}

void ACXPawn::InputPlaceMine(const FInputActionValue& Value)
{
	if (IsGamePlaying() == false)
	{
		return;
	}

	CTSPlaceMine();
}

void ACXPawn::CTSPlaceMine_Implementation()
{
	if (IsGamePlaying() == false)
	{
		return;
	}

	UE_LOG(LogTemplateVehicle, Warning, TEXT("PlaceMine requested by %s"), *GetNameSafe(this));

	UWorld* World = GetWorld();
	if (IsValid(World) == false || IsValid(MineClass) == false)
	{
		return;
	}

	const FVector SpawnLocation = GetActorLocation() - GetActorForwardVector() * 400.0f;
	const FRotator SpawnRotation = FRotator(0.0f, GetActorRotation().Yaw, 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	World->SpawnActor<ACXMine>(MineClass, SpawnLocation, SpawnRotation, SpawnParams);
}

void ACXPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bThrottleBroken);
}

void ACXPawn::ApplyThrottleBreak(float Duration)
{
	if (HasAuthority() == false)
	{
		return;
	}

	bThrottleBroken = true;
	ChaosVehicleMovement->SetThrottleInput(0.0f);

	GetWorldTimerManager().SetTimer(ThrottleBreakTimerHandle, this, &ACXPawn::RecoverThrottle, Duration, false);

	UE_LOG(LogTemplateVehicle, Warning, TEXT("%s throttle broken for %.1f sec"), *GetNameSafe(this), Duration);
}

void ACXPawn::OnRep_ThrottleBroken()
{
	if (bThrottleBroken == true)
	{
		ChaosVehicleMovement->SetThrottleInput(0.0f);
	}
}

void ACXPawn::RecoverThrottle()
{
	bThrottleBroken = false;

	UE_LOG(LogTemplateVehicle, Warning, TEXT("%s throttle recovered"), *GetNameSafe(this));
}

void ACXPawn::MulticastHitEffect_Implementation(const FVector& TraceStart, const FVector& TraceEnd, bool bHitVehicle, const FVector& ImpactPoint, const FVector& ImpactNormal)
{
	UWorld* World = GetWorld();
	DrawDebugLine(World, TraceStart, TraceEnd, FColor::Red, false, 1.0f, 0, 3.0f);

	if (!bHitVehicle)
	{
		return;
	}

	UGameplayStatics::SpawnEmitterAtLocation(World, HitParticle, ImpactPoint, ImpactNormal.Rotation());
}

void ACXPawn::CTSAttack_Implementation()
{
	if (IsGamePlaying() == false)
	{
		return;
	}

	const FVector Start = GetActorLocation();
	const FVector End = Start + GetActorForwardVector() * 700.0f;

	UWorld* World = GetWorld();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHitVehicle = false;
	FHitResult HitResult;

	if(World->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, Params))
	{
		AActor* Actor = HitResult.GetActor();
		UE_LOG(LogTemplateVehicle, Warning, TEXT("Attack trace hit: %s"), *GetNameSafe(HitResult.GetActor()));

		if (ACXPawn* CXPawn = Cast<ACXPawn>(Actor))
		{
			bHitVehicle = true;
			const FVector HitDirection = (End - Start).GetSafeNormal();
			UGameplayStatics::ApplyPointDamage(CXPawn, 5.0f, HitDirection, HitResult, GetController(), this, nullptr);
		}
		
	}

	MulticastHitEffect(Start, End, bHitVehicle, HitResult.ImpactPoint, HitResult.ImpactNormal);
}

#undef LOCTEXT_NAMESPACE