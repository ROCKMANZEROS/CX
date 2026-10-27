// CXPlayerController.cpp


#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "GameFramework/PlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/ChatSubsystem.h"
#include "EngineUtils.h"

ACXPlayerController::ACXPlayerController()
{
	//최소 업데이트 빈도
	MinNetUpdateFrequency = 100.0f;
}

void ACXPlayerController::STCChatLog_Implementation(const TArray<FChatData>& ChatLog)
{
	// Client 시점
	// 신규 접속자가 서버로 부터 채팅 로그를 받음
	if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
	{
		ChatSubsystem->SetChatLog(ChatLog);
	}
}

void ACXPlayerController::CTSChatMessage_Implementation(const FString& Message)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);

	const FString PlayerName = PlayerState->GetPlayerName();
	if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
	{
		ChatSubsystem->AddChatMessage(PlayerName, Message);
	}

	for (TActorIterator<ACXPlayerController> It(GetWorld()); It; ++It)
	{
		if (ACXPlayerController* PC = *It)
		{
			PC->STCChatMessage(PlayerName, Message);
		}
	}
}

void ACXPlayerController::STCChatMessage_Implementation(const FString& Sender, const FString& Message)
{
	UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *Sender, *Message);

	VehicleUI->OnNewChatMessage(Sender, Message);
}

void ACXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}
	
	// spawn the UI widget and add it to the viewport
	VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass);

	check(VehicleUI);
	VehicleUI->AddToViewport();

	if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
	{
		const TArray<FChatData>& ChatLog = ChatSubsystem->GetChatLog();

		for (const FChatData& ChatData : ChatLog)
		{
			VehicleUI->OnNewChatMessage(ChatData.Sender, ChatData.Message);
		}

		ChatSubsystem->ClearChatLog();
	}
}

void ACXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);

		// optionally add the steering wheel context
		if (bUseSteeringWheelControls && SteeringWheelInputMappingContext)
		{
			Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 1);
		}
	}
}

void ACXPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, VehiclePawn);
}

void ACXPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
	}
}

void ACXPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// get a pointer to the controlled pawn
	VehiclePawn = CastChecked<ACXPawn>(InPawn);
}
