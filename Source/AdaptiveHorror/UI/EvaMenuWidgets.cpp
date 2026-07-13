#include "UI/EvaMenuWidgets.h"
#include "AdaptiveHorror.h"
#include "Characters/EvaPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UEvaMenuWidgetBase::RebuildWidget()
{
    bNativeConstructCalled = false;
    InitialFocusButton = nullptr;

    if (WidgetTree)
    {
        UVerticalBox* RootBox = BuildMenuRoot(GetMenuTitleText(), GetMenuSubtitleText());
        if (RootBox)
        {
            BuildMenuContent(RootBox);
        }
    }

    return Super::RebuildWidget();
}

void UEvaMenuWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();
    bNativeConstructCalled = true;

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[TitleUI] NativeConstructCalled Widget=%s RootWidgetValid=%s Visibility=%s RenderOpacity=%.2f"),
        *GetName(),
        HasNativeMenuRoot() ? TEXT("true") : TEXT("false"),
        *UEnum::GetValueAsString(GetVisibility()),
        GetRenderOpacity());
}

bool UEvaMenuWidgetBase::HasNativeMenuRoot() const
{
    return WidgetTree && WidgetTree->RootWidget != nullptr;
}

void UEvaMenuWidgetBase::SetInitialFocusButton(UButton* Button)
{
    InitialFocusButton = Button;
}

bool UEvaMenuWidgetBase::AssignInitialFocus()
{
    APlayerController* OwningController = GetOwningPlayer();
    if (!OwningController || !InitialFocusButton)
    {
        return false;
    }

    InitialFocusButton->SetUserFocus(OwningController);
    InitialFocusButton->SetKeyboardFocus();
    return true;
}

UVerticalBox* UEvaMenuWidgetBase::BuildMenuRoot(const FText& Title, const FText& Subtitle)
{
    if (!WidgetTree)
    {
        return nullptr;
    }

    UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuBackground"));
    Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.86f));
    Background->SetPadding(FMargin(0.0f));

    UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuRoot"));
    Background->SetContent(RootBox);
    WidgetTree->RootWidget = Background;

    AddMenuText(RootBox, Title, 46.0f, FLinearColor(0.78f, 1.0f, 0.92f, 1.0f));
    if (!Subtitle.IsEmpty())
    {
        AddMenuText(RootBox, Subtitle, 16.0f, FLinearColor(0.65f, 0.82f, 0.78f, 1.0f));
    }
    return RootBox;
}

UTextBlock* UEvaMenuWidgetBase::AddMenuText(UVerticalBox* RootBox, const FText& Text, const float FontSize,
    const FLinearColor& Color)
{
    if (!WidgetTree || !RootBox)
    {
        return nullptr;
    }

    UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    TextBlock->SetText(Text);
    TextBlock->SetColorAndOpacity(FSlateColor(Color));
    TextBlock->SetJustification(ETextJustify::Center);
    TextBlock->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FontSize));

    if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(TextBlock))
    {
        VerticalSlot->SetHorizontalAlignment(HAlign_Center);
        VerticalSlot->SetPadding(FMargin(24.0f, FontSize > 30.0f ? 90.0f : 8.0f, 24.0f, 8.0f));
    }
    return TextBlock;
}

UButton* UEvaMenuWidgetBase::AddMenuButton(UVerticalBox* RootBox, const FText& Label, const bool bEnabled)
{
    if (!WidgetTree || !RootBox)
    {
        return nullptr;
    }

    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    Button->SetIsEnabled(bEnabled);

    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    LabelText->SetText(Label);
    LabelText->SetJustification(ETextJustify::Center);
    LabelText->SetColorAndOpacity(FSlateColor(bEnabled ? FLinearColor::White : FLinearColor(0.42f, 0.42f, 0.42f, 1.0f)));
    LabelText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 18.0f));
    Button->SetContent(LabelText);

    if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(Button))
    {
        VerticalSlot->SetHorizontalAlignment(HAlign_Center);
        VerticalSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 8.0f));
    }
    return Button;
}

AEvaPlayerController* UEvaMenuWidgetBase::GetEvaPlayerController() const
{
    return Cast<AEvaPlayerController>(GetOwningPlayer());
}

FText UEvaTitleMenuWidget::GetMenuTitleText() const
{
    return FText::FromString(TEXT("ADAPTIVE HORROR"));
}

FText UEvaTitleMenuWidget::GetMenuSubtitleText() const
{
    return FText::FromString(TEXT("EVA observes. The facility adapts."));
}

void UEvaTitleMenuWidget::BuildMenuContent(UVerticalBox* RootBox)
{
    if (!RootBox)
    {
        return;
    }

    if (UButton* NewGameButton = AddMenuButton(RootBox, FText::FromString(TEXT("NEW GAME"))))
    {
        SetInitialFocusButton(NewGameButton);
        NewGameButton->OnClicked.AddDynamic(this, &UEvaTitleMenuWidget::HandleNewGameClicked);
    }
    AddMenuButton(RootBox, FText::FromString(TEXT("CONTINUE  -  Not Available")), false);
    if (UButton* SettingsButton = AddMenuButton(RootBox, FText::FromString(TEXT("SETTINGS"))))
    {
        SettingsButton->OnClicked.AddDynamic(this, &UEvaTitleMenuWidget::HandleSettingsClicked);
    }
    if (UButton* ExitButton = AddMenuButton(RootBox, FText::FromString(TEXT("EXIT"))))
    {
        ExitButton->OnClicked.AddDynamic(this, &UEvaTitleMenuWidget::HandleExitClicked);
    }
}

void UEvaTitleMenuWidget::HandleNewGameClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->StartNewGameFromMenu();
    }
}

void UEvaTitleMenuWidget::HandleSettingsClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->OpenSettingsMenu(EEvaSettingsReturnTarget::Title);
    }
}

void UEvaTitleMenuWidget::HandleExitClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->ExitGameFromMenu();
    }
}

FText UEvaPauseMenuWidget::GetMenuTitleText() const
{
    return FText::FromString(TEXT("PAUSED"));
}

FText UEvaPauseMenuWidget::GetMenuSubtitleText() const
{
    return FText::FromString(TEXT("The experiment waits."));
}

void UEvaPauseMenuWidget::BuildMenuContent(UVerticalBox* RootBox)
{
    if (!RootBox)
    {
        return;
    }

    if (UButton* ResumeButton = AddMenuButton(RootBox, FText::FromString(TEXT("RESUME"))))
    {
        SetInitialFocusButton(ResumeButton);
        ResumeButton->OnClicked.AddDynamic(this, &UEvaPauseMenuWidget::HandleResumeClicked);
    }
    if (UButton* RestartButton = AddMenuButton(RootBox, FText::FromString(TEXT("RESTART FROM CHECKPOINT"))))
    {
        RestartButton->OnClicked.AddDynamic(this, &UEvaPauseMenuWidget::HandleRestartCheckpointClicked);
    }
    if (UButton* SettingsButton = AddMenuButton(RootBox, FText::FromString(TEXT("SETTINGS"))))
    {
        SettingsButton->OnClicked.AddDynamic(this, &UEvaPauseMenuWidget::HandleSettingsClicked);
    }
    if (UButton* TitleButton = AddMenuButton(RootBox, FText::FromString(TEXT("RETURN TO TITLE"))))
    {
        TitleButton->OnClicked.AddDynamic(this, &UEvaPauseMenuWidget::HandleReturnToTitleClicked);
    }
    if (UButton* ExitButton = AddMenuButton(RootBox, FText::FromString(TEXT("EXIT GAME"))))
    {
        ExitButton->OnClicked.AddDynamic(this, &UEvaPauseMenuWidget::HandleExitClicked);
    }
}

void UEvaPauseMenuWidget::HandleResumeClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->ResumeGameFromMenu();
    }
}

void UEvaPauseMenuWidget::HandleRestartCheckpointClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->RetryFromCheckpointFromMenu();
    }
}

void UEvaPauseMenuWidget::HandleSettingsClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->OpenSettingsMenu(EEvaSettingsReturnTarget::Pause);
    }
}

void UEvaPauseMenuWidget::HandleReturnToTitleClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->ReturnToTitleFromMenu();
    }
}

void UEvaPauseMenuWidget::HandleExitClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->ExitGameFromMenu();
    }
}

FText UEvaGameOverWidget::GetMenuTitleText() const
{
    return FText::FromString(TEXT("GAME OVER"));
}

FText UEvaGameOverWidget::GetMenuSubtitleText() const
{
    return FText::FromString(TEXT("EVA logs another failed survival branch."));
}

void UEvaGameOverWidget::BuildMenuContent(UVerticalBox* RootBox)
{
    if (!RootBox)
    {
        return;
    }

    AddMenuText(RootBox, FText::FromString(TEXT("Death cause and combat statistics are recorded in the HUD/debug log.")),
        14.0f, FLinearColor(0.95f, 0.62f, 0.62f, 1.0f));
    if (UButton* RetryButton = AddMenuButton(RootBox, FText::FromString(TEXT("RETRY FROM CHECKPOINT"))))
    {
        SetInitialFocusButton(RetryButton);
        RetryButton->OnClicked.AddDynamic(this, &UEvaGameOverWidget::HandleRetryClicked);
    }
    if (UButton* RestartButton = AddMenuButton(RootBox, FText::FromString(TEXT("RESTART"))))
    {
        RestartButton->OnClicked.AddDynamic(this, &UEvaGameOverWidget::HandleRestartClicked);
    }
    if (UButton* TitleButton = AddMenuButton(RootBox, FText::FromString(TEXT("RETURN TO TITLE"))))
    {
        TitleButton->OnClicked.AddDynamic(this, &UEvaGameOverWidget::HandleReturnToTitleClicked);
    }
}

void UEvaGameOverWidget::HandleRetryClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->RetryFromCheckpointFromMenu();
    }
}

void UEvaGameOverWidget::HandleRestartClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->StartNewGameFromMenu();
    }
}

void UEvaGameOverWidget::HandleReturnToTitleClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->ReturnToTitleFromMenu();
    }
}

FText UEvaStageClearWidget::GetMenuTitleText() const
{
    return FText::FromString(TEXT("MISSION COMPLETE"));
}

FText UEvaStageClearWidget::GetMenuSubtitleText() const
{
    return FText::FromString(TEXT("Research Facility Demo - ADAM defeated."));
}

void UEvaStageClearWidget::BuildMenuContent(UVerticalBox* RootBox)
{
    if (!RootBox)
    {
        return;
    }

    AddMenuText(RootBox, FText::FromString(TEXT("Stage results are visible in the HUD and DEV log. NEXT STAGE: COMING SOON.")),
        14.0f, FLinearColor(0.58f, 1.0f, 0.68f, 1.0f));
    if (UButton* RetryButton = AddMenuButton(RootBox, FText::FromString(TEXT("RETRY"))))
    {
        SetInitialFocusButton(RetryButton);
        RetryButton->OnClicked.AddDynamic(this, &UEvaStageClearWidget::HandleRetryClicked);
    }
    if (UButton* TitleButton = AddMenuButton(RootBox, FText::FromString(TEXT("RETURN TO TITLE"))))
    {
        TitleButton->OnClicked.AddDynamic(this, &UEvaStageClearWidget::HandleReturnToTitleClicked);
    }
    if (UButton* ExitButton = AddMenuButton(RootBox, FText::FromString(TEXT("EXIT GAME"))))
    {
        ExitButton->OnClicked.AddDynamic(this, &UEvaStageClearWidget::HandleExitClicked);
    }
}

void UEvaStageClearWidget::HandleRetryClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->StartNewGameFromMenu();
    }
}

void UEvaStageClearWidget::HandleReturnToTitleClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->ReturnToTitleFromMenu();
    }
}

void UEvaStageClearWidget::HandleExitClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->ExitGameFromMenu();
    }
}

FText UEvaSettingsWidget::GetMenuTitleText() const
{
    return FText::FromString(TEXT("SETTINGS"));
}

FText UEvaSettingsWidget::GetMenuSubtitleText() const
{
    return FText::FromString(TEXT("Prototype settings are saved locally."));
}

void UEvaSettingsWidget::BuildMenuContent(UVerticalBox* RootBox)
{
    if (!RootBox)
    {
        return;
    }

    AEvaPlayerController* Controller = GetEvaPlayerController();
    const float Master = Controller ? Controller->GetMasterVolumeSetting() : 0.8f;
    const float BGM = Controller ? Controller->GetBGMVolumeSetting() : 0.45f;
    const float SFX = Controller ? Controller->GetSFXVolumeSetting() : 0.85f;
    const float Sensitivity = Controller ? Controller->GetMouseSensitivitySetting() : 1.0f;
    const bool bInvert = Controller && Controller->IsMouseYInvertedSetting();

    ValuesText = AddMenuText(RootBox, FText::GetEmpty(), 14.0f, FLinearColor(0.72f, 0.95f, 1.0f, 1.0f));
    AddLabeledSlider(RootBox, TEXT("Master Volume"), Master, FLinearColor::White, MasterVolumeSlider);
    AddLabeledSlider(RootBox, TEXT("BGM Volume"), BGM, FLinearColor::White, BGMVolumeSlider);
    AddLabeledSlider(RootBox, TEXT("SFX Volume"), SFX, FLinearColor::White, SFXVolumeSlider);
    AddLabeledSlider(RootBox, TEXT("Mouse Sensitivity"), (Sensitivity - 0.1f) / 4.9f,
        FLinearColor::White, MouseSensitivitySlider);

    InvertYCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("InvertYCheckBox"));
    InvertYCheckBox->SetIsChecked(bInvert);
    UTextBlock* InvertLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    InvertLabel->SetText(FText::FromString(TEXT("Invert Mouse Y")));
    InvertLabel->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 16.0f));
    InvertYCheckBox->SetContent(InvertLabel);
    InvertYCheckBox->OnCheckStateChanged.AddDynamic(this, &UEvaSettingsWidget::HandleInvertYChanged);
    if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(InvertYCheckBox))
    {
        VerticalSlot->SetHorizontalAlignment(HAlign_Center);
        VerticalSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 8.0f));
    }

    if (MasterVolumeSlider)
    {
        MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UEvaSettingsWidget::HandleMasterVolumeChanged);
    }
    if (BGMVolumeSlider)
    {
        BGMVolumeSlider->OnValueChanged.AddDynamic(this, &UEvaSettingsWidget::HandleBGMVolumeChanged);
    }
    if (SFXVolumeSlider)
    {
        SFXVolumeSlider->OnValueChanged.AddDynamic(this, &UEvaSettingsWidget::HandleSFXVolumeChanged);
    }
    if (MouseSensitivitySlider)
    {
        MouseSensitivitySlider->OnValueChanged.AddDynamic(this, &UEvaSettingsWidget::HandleMouseSensitivityChanged);
    }

    if (UButton* ApplyButton = AddMenuButton(RootBox, FText::FromString(TEXT("APPLY"))))
    {
        ApplyButton->OnClicked.AddDynamic(this, &UEvaSettingsWidget::HandleApplyClicked);
    }
    if (UButton* BackButton = AddMenuButton(RootBox, FText::FromString(TEXT("BACK"))))
    {
        BackButton->OnClicked.AddDynamic(this, &UEvaSettingsWidget::HandleBackClicked);
    }

    RefreshValueText();
}

UTextBlock* UEvaSettingsWidget::AddLabeledSlider(UVerticalBox* RootBox, const FString& Label, const float Value,
    const FLinearColor Color, TObjectPtr<USlider>& OutSlider)
{
    AddMenuText(RootBox, FText::FromString(Label), 14.0f, Color);
    OutSlider = WidgetTree ? WidgetTree->ConstructWidget<USlider>(USlider::StaticClass()) : nullptr;
    if (OutSlider)
    {
        OutSlider->SetValue(FMath::Clamp(Value, 0.0f, 1.0f));
        if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(OutSlider))
        {
            VerticalSlot->SetHorizontalAlignment(HAlign_Center);
            VerticalSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
        }
    }
    return nullptr;
}

void UEvaSettingsWidget::RefreshValueText()
{
    AEvaPlayerController* Controller = GetEvaPlayerController();
    if (!ValuesText || !Controller)
    {
        return;
    }

    ValuesText->SetText(FText::FromString(FString::Printf(
        TEXT("Master %.0f%% | BGM %.0f%% | SFX %.0f%% | Mouse %.2f | Invert Y %s | Windowed | 1280x720 | Quality Prototype"),
        Controller->GetMasterVolumeSetting() * 100.0f,
        Controller->GetBGMVolumeSetting() * 100.0f,
        Controller->GetSFXVolumeSetting() * 100.0f,
        Controller->GetMouseSensitivitySetting(),
        Controller->IsMouseYInvertedSetting() ? TEXT("ON") : TEXT("OFF"))));
}

void UEvaSettingsWidget::HandleMasterVolumeChanged(const float Value)
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->SetMasterVolumeSetting(Value);
        RefreshValueText();
    }
}

void UEvaSettingsWidget::HandleBGMVolumeChanged(const float Value)
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->SetBGMVolumeSetting(Value);
        RefreshValueText();
    }
}

void UEvaSettingsWidget::HandleSFXVolumeChanged(const float Value)
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->SetSFXVolumeSetting(Value);
        RefreshValueText();
    }
}

void UEvaSettingsWidget::HandleMouseSensitivityChanged(const float Value)
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->SetMouseSensitivitySetting(0.1f + FMath::Clamp(Value, 0.0f, 1.0f) * 4.9f);
        RefreshValueText();
    }
}

void UEvaSettingsWidget::HandleInvertYChanged(const bool bIsChecked)
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->SetInvertMouseYSetting(bIsChecked);
        RefreshValueText();
    }
}

void UEvaSettingsWidget::HandleApplyClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->SaveAndApplySettings();
        RefreshValueText();
    }
}

void UEvaSettingsWidget::HandleBackClicked()
{
    if (AEvaPlayerController* Controller = GetEvaPlayerController())
    {
        Controller->ReturnFromSettingsMenu(ReturnTarget);
    }
}
