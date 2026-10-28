// CXGameMode.cpp

#include "CXGameMode.h"
#include "CXPlayerController.h"
#include "PlayerState/CXPlayerState.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Subsystem/ChatSubsystem.h"
#include "GameState/CXGameState.h"

ACXGameMode::ACXGameMode()
{
	PlayerControllerClass = ACXPlayerController::StaticClass();
	PlayerStateClass = ACXPlayerState::StaticClass();
	GameStateClass = ACXGameState::StaticClass();
}

void ACXGameMode::BeginPlay()
{
	Super::BeginPlay();

	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	if (IsValid(CXGameState))
	{
		CXGameState->SetGameState(EGameState::Waiting);
		CXGameState->SetRemainingTime(10);
	}
}

void ACXGameMode::StartWaitingCountdown()
{
	bWaitingCountdownStarted = true;

	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	CXGameState->SetGameState(EGameState::Waiting);
	CXGameState->SetRemainingTime(10);
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ACXGameMode::HandleWaitingTimerTick, 1.0f, true);
}

void ACXGameMode::HandleWaitingTimerTick()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	const int32 NewRemainingTime = CXGameState->GetRemainingTime() - 1;
	CXGameState->SetRemainingTime(NewRemainingTime);

	if (NewRemainingTime <= 0)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		StartPlayingState();
	}
}

void ACXGameMode::StartPlayingState()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	CXGameState->SetGameState(EGameState::Playing);
	CXGameState->SetRemainingTime(100);
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ACXGameMode::HandlePlayingTimerTick, 1.0f, true);
}

void ACXGameMode::HandlePlayingTimerTick()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();
	const int32 NewRemainingTime = CXGameState->GetRemainingTime() - 1;
	CXGameState->SetRemainingTime(NewRemainingTime);

	if (NewRemainingTime <= 0)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		FinishGame();
	}
}

void ACXGameMode::FinishGame()
{
	ACXGameState* CXGameState = GetGameState<ACXGameState>();

	int32 HighestCoins = -1;
	TArray<ACXPlayerState*> Winners;

	for (APlayerState* PlayerState : CXGameState->PlayerArray)
	{
		if (ACXPlayerState* CXPlayerState = Cast<ACXPlayerState>(PlayerState))
		{
			const int32 PlayerCoins = CXPlayerState->GetCoins();
			if (HighestCoins < PlayerCoins)
			{
				HighestCoins = PlayerCoins;
				Winners.Empty();
				Winners.Add(CXPlayerState);
			}
			else if (HighestCoins == PlayerCoins)
			{
				Winners.Add(CXPlayerState);
			}
		}
	}
	CXGameState->Winners = Winners;
	CXGameState->SetRemainingTime(0);
	CXGameState->SetGameState(EGameState::Finished);
}

void ACXGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (IsValid(NewPlayer) == false)
	{
		return;
	}

	if (ACXPlayerController* CXPlayerController = Cast<ACXPlayerController>(NewPlayer))
	{
		// 서버 시점에 클라로 채팅 로그를 보내줌
		if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
		{
			const TArray<FChatData> ChatLog = ChatSubsystem->GetChatLog();
			if (!ChatLog.IsEmpty())
			{
				CXPlayerController->STCChatLog(ChatLog);
			}
		}
	}

	if (bWaitingCountdownStarted == false)
	{
		StartWaitingCountdown();
	}

}

AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> AvailablePlayerStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (IsPlayerStartAvailable(PlayerStart))
		{
			AvailablePlayerStarts.Add(PlayerStart);
		}
	}

	checkf(AvailablePlayerStarts.IsEmpty() == false, TEXT("There is no available Vehicle PlayerStart."));

	const int32 RandomIndex = FMath::RandHelper(AvailablePlayerStarts.Num());
	return Cast<AActor>(AvailablePlayerStarts[RandomIndex]);
}

bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* PlayerStart) const
{
	if (IsValid(PlayerStart) == false || IsValid(GetWorld()) == false)
	{
		return false;
	}

	const FTransform PlayerStartTransform = PlayerStart->GetActorTransform();
	const FVector CheckLocation = PlayerStartTransform.TransformPosition(VehicleSpawnCheckOffset);
	const FQuat CheckRotation = PlayerStartTransform.GetRotation();
	const FCollisionShape CheckShape = FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VehiclePlayerStartCheck), false);

	const bool bVehicleExists =
		GetWorld()->OverlapAnyTestByObjectType(
			CheckLocation,
			CheckRotation,
			ObjectQueryParams,
			CheckShape,
			QueryParams
		);

	return !bVehicleExists;
}