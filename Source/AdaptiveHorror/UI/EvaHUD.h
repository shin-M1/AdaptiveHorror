#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EvaHUD.generated.h"

class UEvaBossHUDWidget;
class AEvaPlayerCharacter;
class AEvaPrototypeGameMode;

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void DrawHUD() override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Boss HUD")
    TSubclassOf<UEvaBossHUDWidget> BossHUDWidgetClass;

private:
    void EnsureBossHUDWidget();
    void UpdateBossHUD();
    void DrawHorrorOverlay(const AEvaPlayerCharacter* Player, const AEvaPrototypeGameMode* GameMode);

    UPROPERTY()
    TObjectPtr<UEvaBossHUDWidget> BossHUDWidget;
};
