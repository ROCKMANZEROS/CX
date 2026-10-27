#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXUI.generated.h"

class UChatUI;
class UTextBlock;

UCLASS(abstract)
class CX_API UCXUI : public UUserWidget
{
	GENERATED_BODY()

#pragma region ChatUI
public:
	void OnNewChatMessage(const FString& InSender, const FString& InMessage);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UChatUI> UI_Chat;
#pragma endregion
	
protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Vehicle)
	bool bIsMPH = false;

public:

	void UpdateSpeed(float NewSpeed);

	void UpdateGear(int32 NewGear);

protected:

	
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnSpeedUpdate(float NewSpeed);

	
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnGearUpdate(int32 NewGear);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_RemainingTime;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_GameState;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_Result;
};
