#include "UI/EvaBossHUDWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "Rendering/DrawElementTypes.h"
#include "Styling/CoreStyle.h"

void UEvaBossHUDWidget::SetBossSnapshot(const bool bInBossActive, const FString& InBossName,
    const float InHealthPercent, const float InCurrentHealth, const float InMaxHealth, const FString& InPhaseText,
    const FString& InCurrentState, const float InTargetDistance, const int32 InSummonCount,
    const FString& InLastEvent, const bool bInDebugVisible)
{
    bBossActive = bInBossActive;
    bDebugVisible = bInDebugVisible;
    BossName = InBossName.IsEmpty() ? TEXT("ADAM") : InBossName;
    HealthPercent = FMath::Clamp(InHealthPercent, 0.0f, 1.0f);
    CurrentHealth = FMath::Max(0.0f, InCurrentHealth);
    MaxHealth = FMath::Max(0.0f, InMaxHealth);
    PhaseText = InPhaseText.IsEmpty() ? TEXT("Phase 1") : InPhaseText;
    CurrentState = InCurrentState.IsEmpty() ? TEXT("Unknown") : InCurrentState;
    TargetDistance = InTargetDistance;
    SummonCount = FMath::Max(0, InSummonCount);
    LastEvent = InLastEvent.IsEmpty() ? TEXT("None") : InLastEvent;
}

void UEvaBossHUDWidget::ClearBossSnapshot()
{
    bBossActive = false;
    bDebugVisible = false;
    HealthPercent = 0.0f;
    CurrentHealth = 0.0f;
    MaxHealth = 0.0f;
    TargetDistance = -1.0f;
    SummonCount = 0;
    CurrentState = TEXT("Inactive");
    LastEvent = TEXT("None");
}

int32 UEvaBossHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, const int32 LayerId,
    const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
    int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId,
        InWidgetStyle, bParentEnabled);

    if (!bBossActive)
    {
        return MaxLayer;
    }

    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    const FVector2D ActivePanelSize = bDebugVisible ? PanelSize : CompactPanelSize;
    const float PanelX = FMath::Max(12.0f, (LocalSize.X - ActivePanelSize.X) * 0.5f);
    const FVector2D PanelPosition(PanelX, 24.0f);
    const FVector2D BarPosition(PanelPosition.X + 34.0f, PanelPosition.Y + 48.0f);
    const FVector2D BarSize(ActivePanelSize.X - 68.0f, 18.0f);
    const FVector2D FillSize(BarSize.X * HealthPercent, BarSize.Y);

    DrawBox(OutDrawElements, AllottedGeometry, ++MaxLayer, PanelPosition, ActivePanelSize,
        FLinearColor(0.02f, 0.02f, 0.025f, 0.78f));
    DrawBox(OutDrawElements, AllottedGeometry, ++MaxLayer, BarPosition, BarSize,
        FLinearColor(0.08f, 0.01f, 0.01f, 0.95f));
    DrawBox(OutDrawElements, AllottedGeometry, ++MaxLayer, BarPosition, FillSize,
        HealthPercent <= 0.25f ? FLinearColor(1.0f, 0.05f, 0.02f, 1.0f) :
        FLinearColor(0.95f, 0.18f, 0.08f, 1.0f));

    DrawString(OutDrawElements, AllottedGeometry, ++MaxLayer, PanelPosition + FVector2D(32.0f, 13.0f),
        BossName, 24.0f, FLinearColor(1.0f, 0.22f, 0.12f, 1.0f), TEXT("Bold"));
    DrawString(OutDrawElements, AllottedGeometry, MaxLayer, PanelPosition + FVector2D(ActivePanelSize.X - 132.0f, 17.0f),
        PhaseText, 16.0f, FLinearColor(1.0f, 0.78f, 0.35f, 1.0f), TEXT("Bold"));

    if (bDebugVisible)
    {
        const float TextY = PanelPosition.Y + 76.0f;
        DrawString(OutDrawElements, AllottedGeometry, ++MaxLayer, PanelPosition + FVector2D(32.0f, TextY - PanelPosition.Y),
            FString::Printf(TEXT("HP: %.0f / %.0f    Remaining HP: %.0f"), CurrentHealth, MaxHealth, CurrentHealth),
            14.0f, FLinearColor::White);
        DrawString(OutDrawElements, AllottedGeometry, MaxLayer, PanelPosition + FVector2D(32.0f, TextY - PanelPosition.Y + 21.0f),
            FString::Printf(TEXT("State: %s    Target Distance: %.0f cm"), *CurrentState, TargetDistance),
            14.0f, FLinearColor(0.80f, 0.95f, 1.0f, 1.0f));
        DrawString(OutDrawElements, AllottedGeometry, MaxLayer, PanelPosition + FVector2D(32.0f, TextY - PanelPosition.Y + 42.0f),
            FString::Printf(TEXT("Last Event: %s    Summon Count: %d"), *LastEvent, SummonCount),
            14.0f, FLinearColor(1.0f, 0.82f, 0.42f, 1.0f));
    }

    return MaxLayer;
}

void UEvaBossHUDWidget::DrawBox(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry,
    const int32 Layer, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color) const
{
    if (Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return;
    }

    const FSlateBrush* Brush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    FSlateDrawElement::MakeBox(OutDrawElements, Layer,
        AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)), Brush,
        ESlateDrawEffect::None, Color);
}

void UEvaBossHUDWidget::DrawString(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry,
    const int32 Layer, const FVector2D& Position, const FString& Text, const float FontSize,
    const FLinearColor& Color, const FName Typeface) const
{
    const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle(Typeface, FontSize);
    FSlateDrawElement::MakeText(OutDrawElements, Layer, AllottedGeometry.ToOffsetPaintGeometry(Position),
        Text, FontInfo, ESlateDrawEffect::None, Color);
}
