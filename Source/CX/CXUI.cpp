#include "CXUI.h"
#include "UI/ChatUI.h"
#include "Components/TextBlock.h"
#include "GameState/CXGameState.h"
#include "PlayerState/CXPlayerState.h"

void UCXUI::OnNewChatMessage(const FString& InSender, const FString& InMessage)
{
	UI_Chat->OnNewChatMessage(InSender, InMessage);
}

void UCXUI::UpdateSpeed(float NewSpeed)
{
	
	float FormattedSpeed = FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	OnSpeedUpdate(FormattedSpeed);
}

void UCXUI::UpdateGear(int32 NewGear)
{
	
	OnGearUpdate(NewGear);
}

void UCXUI::NativeConstruct()
{
	Super::NativeConstruct();

	TextBlock_Result->SetVisibility(ESlateVisibility::Collapsed);
}

void UCXUI::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return;
	}

	ACXGameState* CXGameState = World->GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	// 남은 시간
	TextBlock_RemainingTime->SetText(FText::AsNumber(CXGameState->GetRemainingTime()));

	// 게임 상태
	const EGameState CurrentState = CXGameState->GetCurrentGameState();

	FString StateString = TEXT("");
	if (CurrentState == EGameState::Waiting)
	{
		StateString = TEXT("대기 중");
	}
	else if (CurrentState == EGameState::Playing)
	{
		StateString = TEXT("진행 중");
	}
	else if (CurrentState == EGameState::Finished)
	{
		StateString = TEXT("종료");
	}
	TextBlock_GameState->SetText(FText::FromString(StateString));

	// 승리/패배 (종료됐을 때만)
	if (CurrentState != EGameState::Finished)
	{
		TextBlock_Result->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ACXPlayerState* LocalPlayerState = GetOwningPlayerState<ACXPlayerState>();
	if (IsValid(LocalPlayerState) == false)
	{
		return;
	}

	FString ResultString = TEXT("패배...");
	if (CXGameState->Winners.Contains(LocalPlayerState) == true)
	{
		ResultString = TEXT("승리!");
	}

	TextBlock_Result->SetText(FText::FromString(ResultString));
	TextBlock_Result->SetVisibility(ESlateVisibility::Visible);
}
