// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/CXPlayerState.h"
#include "Net/UnrealNetwork.h"

static int32 GMyTestCoin = 0;
// 2. FAutoConsoleVariableRef 정의 및 람다 콜백 등록
static FAutoConsoleVariableRef CvarAddCoinServer(
	TEXT("Cheat.addcoin.server"),
	GMyTestCoin,
	TEXT("add coin request to server"),    // 도움말(Help text)
	FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* CVar)    // 람다 콜백
		{
			UE_LOG(LogTemp, Warning, TEXT("콘솔 변수 변경됨! GMyTestCoin 값: %d"), GMyTestCoin);

			if (GWorld)
			{
				if (GWorld->GetFirstPlayerController())
				{
					GWorld->GetFirstPlayerController()->GetPlayerState<ACXPlayerState>()->CTSAddCoin(GMyTestCoin);
				}
			}
			GMyTestCoin = 0;
		}),
	ECVF_Cheat               // 플래그 (치트 기능 등으로 분류)
);

static FAutoConsoleVariableRef CvarAddCoinClient(
	TEXT("Cheat.addcoin.Client"),
	GMyTestCoin,
	TEXT("add coin test for client only"),    // 도움말(Help text)
	FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* CVar)    // 람다 콜백
		{
			UE_LOG(LogTemp, Warning, TEXT("콘솔 변수 변경됨! GMyTestCoin 값: %d"), GMyTestCoin);

			if (GWorld)
			{
				if (GWorld->GetFirstPlayerController())
				{
					GWorld->GetFirstPlayerController()->GetPlayerState<ACXPlayerState>()->Coins += GMyTestCoin;
				}
			}
			GMyTestCoin = 0;
		}),
	ECVF_Cheat               // 플래그 (치트 기능 등으로 분류)
);

void ACXPlayerState::CTSAddCoin_Implementation(const int32& Amount)
{
	// 서버 시점
	AddCoins(Amount);
}

void ACXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, Coins, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ThisClass, BonusMaxSpeed, COND_OwnerOnly);
}

void ACXPlayerState::AddCoins(int32 Amount)
{
	if (HasAuthority() == false || Amount <= 0)
	{
		return;
	}

	FString PlayerName = GetPlayerName();
	Coins += Amount;
	UE_LOG(LogTemp, Warning, TEXT("%s - AddCoins: %d(+%d)"), *PlayerName, Coins, Amount);
	
}

bool ACXPlayerState::TryRemoveCoins(int32 Amount)
{
	if (HasAuthority() == false || Amount <= 0 || Coins < 5)
	{
		return false;
	}

	Coins -= Amount;
	return true;
}

void ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
	if (HasAuthority() == false || Amount == 0)
	{
		return;
	}

	BonusMaxSpeed += Amount;
}

FString ACXPlayerState::GetPlayerStateInfo() const
{
	return FString::Printf(TEXT("Coins: %d, BounsMaxSpeed: %.2f"), Coins, BonusMaxSpeed);
}

void ACXPlayerState::RecoverBonusMaxSpeedPenalty(float Amount)
{
	if (HasAuthority() == false || Amount <= 0.0f)
	{
		return;
	}

	// 패널티(음수)가 없으면 아무것도 하지 않음
	if (BonusMaxSpeed >= 0.0f)
	{
		return;
	}

	// 0을 넘어 양수가 되지 않도록 제한
	BonusMaxSpeed = FMath::Min(BonusMaxSpeed + Amount, 0.0f);

	if (BonusMaxSpeed >= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s - speed penalty fully recovered"), *GetPlayerName());
	}
}
