#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EvaBossHUDWidget.generated.h"

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API UEvaBossHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EVA|Boss HUD")
    void SetBossSnapshot(bool bInBossActive, const FString& InBossName, float InHealthPercent,
        float InCurrentHealth, float InMaxHealth, const FString& InPhaseText, const FString& InCurrentState,
        float InTargetDistance, int32 InSummonCount, const FString& InLastEvent, bool bInDebugVisible);

    UFUNCTION(BlueprintCallable, Category = "EVA|Boss HUD")
    void ClearBossSnapshot();

protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
        const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Boss HUD")
    FVector2D PanelSize = FVector2D(560.0f, 158.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Boss HUD")
    FVector2D CompactPanelSize = FVector2D(520.0f, 92.0f);

private:
    void DrawBox(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, int32 Layer,
        const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color) const;
    void DrawString(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, int32 Layer,
        const FVector2D& Position, const FString& Text, float FontSize, const FLinearColor& Color,
        const FName Typeface = TEXT("Regular")) const;

    UPROPERTY()
    bool bBossActive = false;

    UPROPERTY()
    bool bDebugVisible = false;

    UPROPERTY()
    FString BossName = TEXT("ADAM");

    UPROPERTY()
    FString PhaseText = TEXT("Phase 1");

    UPROPERTY()
    FString CurrentState = TEXT("Inactive");

    UPROPERTY()
    FString LastEvent = TEXT("None");

    UPROPERTY()
    float HealthPercent = 0.0f;

    UPROPERTY()
    float CurrentHealth = 0.0f;

    UPROPERTY()
    float MaxHealth = 0.0f;

    UPROPERTY()
    float TargetDistance = -1.0f;

    UPROPERTY()
    int32 SummonCount = 0;
};
