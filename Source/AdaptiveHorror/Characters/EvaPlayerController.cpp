#include "Characters/EvaPlayerController.h"
#include "AdaptiveHorror.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Core/EvaSettingsSaveGame.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundWaveProcedural.h"
#include "UI/EvaMenuWidgets.h"
#include "Blueprint/UserWidget.h"

const FString AEvaPlayerController::SettingsSlotName = TEXT("EvaPrototypeSettings");

void AEvaPlayerController::BeginPlay()
{
    Super::BeginPlay();
    LoadSettings();
    ApplySettingsToPlayer();

    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->EnterTitleMode();
    }
    else
    {
        ShowTitleMenu();
    }
}

void AEvaPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (InputComponent)
    {
        InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AEvaPlayerController::HandleEscapePressed);
    }
}

void AEvaPlayerController::HandleEscapePressed()
{
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        if (GameMode->GetGameFlowState() == EEvaGameFlowState::Playing)
        {
            GameMode->PauseGameFlow();
        }
        else if (GameMode->GetGameFlowState() == EEvaGameFlowState::Paused)
        {
            GameMode->ResumeGameFlow();
        }
    }
}

bool AEvaPlayerController::ShowTitleMenu()
{
    CloseAllMenus();
    bool bCreateWidgetAttempted = false;
    bool bAddToViewportAttempted = false;
    bool bFocusAssigned = false;
    FString FailureReason = TEXT("None");

    TSubclassOf<UEvaTitleMenuWidget> WidgetClass = TitleMenuWidgetClass;
    if (!WidgetClass.Get())
    {
        WidgetClass = UEvaTitleMenuWidget::StaticClass();
    }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!IsLocalController())
    {
        FailureReason = TEXT("PlayerController is not local.");
    }
    else if (!LocalPlayer)
    {
        FailureReason = TEXT("LocalPlayer is null.");
    }
    else if (!LocalPlayer->ViewportClient)
    {
        FailureReason = TEXT("LocalPlayer ViewportClient is null.");
    }
    else if (!WidgetClass.Get())
    {
        FailureReason = TEXT("TitleWidgetClass is null.");
    }
    else
    {
        bCreateWidgetAttempted = true;
        TitleMenuWidget = CreateWidget<UEvaTitleMenuWidget>(this, WidgetClass);
        if (!TitleMenuWidget)
        {
            FailureReason = TEXT("CreateWidget returned null.");
        }
    }

    if (TitleMenuWidget)
    {
        TitleMenuWidget->SetVisibility(ESlateVisibility::Visible);
        TitleMenuWidget->SetRenderOpacity(1.0f);
        bAddToViewportAttempted = true;
        TitleMenuWidget->AddToViewport(100);
        bFocusAssigned = TitleMenuWidget->AssignInitialFocus();
        if (!TitleMenuWidget->IsInViewport())
        {
            FailureReason = TEXT("AddToViewport completed but IsInViewport is false.");
        }
    }

    ApplyMenuInputMode(true);
    LogTitleUIState(TEXT("ShowTitleMenu"), bCreateWidgetAttempted, bAddToViewportAttempted, bFocusAssigned,
        FailureReason);
    LogInputState(TEXT("ShowTitleMenu"));
    PlayUIEvent(146.8f, 0.18f, 0.30f);

    return TitleMenuWidget && TitleMenuWidget->IsInViewport();
}

void AEvaPlayerController::ShowPauseMenu()
{
    RemoveWidget(SettingsWidget);
    SettingsWidget = nullptr;
    if (!PauseMenuWidget)
    {
        TSubclassOf<UEvaPauseMenuWidget> WidgetClass = PauseMenuWidgetClass;
        if (!WidgetClass.Get())
        {
            WidgetClass = UEvaPauseMenuWidget::StaticClass();
        }
        PauseMenuWidget = CreateWidget<UEvaPauseMenuWidget>(this, WidgetClass);
        if (PauseMenuWidget)
        {
            PauseMenuWidget->AddToViewport(110);
        }
    }
    ApplyMenuInputMode(true);
    if (PauseMenuWidget)
    {
        PauseMenuWidget->AssignInitialFocus();
    }
    LogInputState(TEXT("ShowPauseMenu"));
    PlayUIEvent(220.0f, 0.10f, 0.45f);
}

void AEvaPlayerController::ShowGameOverMenu()
{
    CloseAllMenus();
    TSubclassOf<UEvaGameOverWidget> WidgetClass = GameOverWidgetClass;
    if (!WidgetClass.Get())
    {
        WidgetClass = UEvaGameOverWidget::StaticClass();
    }
    GameOverWidget = CreateWidget<UEvaGameOverWidget>(this, WidgetClass);
    if (GameOverWidget)
    {
        GameOverWidget->AddToViewport(120);
    }
    ApplyMenuInputMode(true);
    if (GameOverWidget)
    {
        GameOverWidget->AssignInitialFocus();
    }
    LogInputState(TEXT("ShowGameOverMenu"));
    PlayUIEvent(82.4f, 0.35f, 0.80f);
}

void AEvaPlayerController::ShowStageClearMenu()
{
    CloseAllMenus();
    TSubclassOf<UEvaStageClearWidget> WidgetClass = StageClearWidgetClass;
    if (!WidgetClass.Get())
    {
        WidgetClass = UEvaStageClearWidget::StaticClass();
    }
    StageClearWidget = CreateWidget<UEvaStageClearWidget>(this, WidgetClass);
    if (StageClearWidget)
    {
        StageClearWidget->AddToViewport(120);
    }
    ApplyMenuInputMode(true);
    if (StageClearWidget)
    {
        StageClearWidget->AssignInitialFocus();
    }
    LogInputState(TEXT("ShowStageClearMenu"));
    PlayUIEvent(523.25f, 0.35f, 0.85f);
}

void AEvaPlayerController::StartNewGameFromMenu()
{
    PlayUIClick();
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->StartNewGameFlow();
    }
}

void AEvaPlayerController::ResumeGameFromMenu()
{
    PlayUIClick();
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->ResumeGameFlow();
    }
}

void AEvaPlayerController::RetryFromCheckpointFromMenu()
{
    PlayUIClick();
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->RetryFromCheckpointFlow();
    }
}

void AEvaPlayerController::ReturnToTitleFromMenu()
{
    PlayUIClick();
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->ReturnToTitleFlow();
    }
}

void AEvaPlayerController::ExitGameFromMenu()
{
    PlayUIClick();
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AEvaPlayerController::OpenSettingsMenu(const EEvaSettingsReturnTarget ReturnTarget)
{
    PlayUIClick();
    RemoveWidget(TitleMenuWidget);
    TitleMenuWidget = nullptr;
    RemoveWidget(PauseMenuWidget);
    PauseMenuWidget = nullptr;
    RemoveWidget(SettingsWidget);
    SettingsWidget = nullptr;

    TSubclassOf<UEvaSettingsWidget> WidgetClass = SettingsWidgetClass;
    if (!WidgetClass.Get())
    {
        WidgetClass = UEvaSettingsWidget::StaticClass();
    }
    SettingsWidget = CreateWidget<UEvaSettingsWidget>(this, WidgetClass);
    if (SettingsWidget)
    {
        SettingsWidget->SetReturnTarget(ReturnTarget);
        SettingsWidget->AddToViewport(130);
    }
    ApplyMenuInputMode(true);
}

void AEvaPlayerController::ReturnFromSettingsMenu(const EEvaSettingsReturnTarget ReturnTarget)
{
    SaveAndApplySettings();
    RemoveWidget(SettingsWidget);
    SettingsWidget = nullptr;
    if (ReturnTarget == EEvaSettingsReturnTarget::Pause)
    {
        ShowPauseMenu();
    }
    else
    {
        ShowTitleMenu();
    }
}

void AEvaPlayerController::ApplyGameplayInputMode()
{
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
    LastAppliedInputMode = TEXT("GameOnly");
    bShowMouseCursor = false;
    ResetIgnoreMoveInput();
    ResetIgnoreLookInput();
    LogInputState(TEXT("ApplyGameplayInputMode"));
}

void AEvaPlayerController::CloseMenusForGameplay()
{
    CloseAllMenus();
}

void AEvaPlayerController::ApplyMenuInputMode(const bool bPauseGameInput)
{
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
    LastAppliedInputMode = TEXT("GameAndUI");
    bShowMouseCursor = true;
    SetIgnoreMoveInput(bPauseGameInput);
    SetIgnoreLookInput(bPauseGameInput);
    LogInputState(bPauseGameInput ? TEXT("ApplyMenuInputMode_BlockGameplay") : TEXT("ApplyMenuInputMode_AllowGameplay"));
}

bool AEvaPlayerController::IsTitleMenuVisibleForDebug() const
{
    return TitleMenuWidget && TitleMenuWidget->IsInViewport() &&
        TitleMenuWidget->GetVisibility() == ESlateVisibility::Visible;
}

void AEvaPlayerController::LogInputState(const FString& Context) const
{
    const UWorld* World = GetWorld();
    const APawn* PossessedPawn = GetPawn();
    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[InputState] Context=%s InputMode=%s ShowMouseCursor=%s IgnoreMoveInput=%s IgnoreLookInput=%s IsPaused=%s GlobalTimeDilation=%.2f PlayerController=%s PossessedPawn=%s LocalController=%s LocalPlayerValid=%s"),
        *Context,
        *LastAppliedInputMode,
        bShowMouseCursor ? TEXT("true") : TEXT("false"),
        IsMoveInputIgnored() ? TEXT("true") : TEXT("false"),
        IsLookInputIgnored() ? TEXT("true") : TEXT("false"),
        World && UGameplayStatics::IsGamePaused(World) ? TEXT("true") : TEXT("false"),
        World ? UGameplayStatics::GetGlobalTimeDilation(World) : 1.0f,
        *GetClass()->GetName(),
        PossessedPawn ? *PossessedPawn->GetClass()->GetName() : TEXT("None"),
        IsLocalController() ? TEXT("true") : TEXT("false"),
        GetLocalPlayer() ? TEXT("true") : TEXT("false"));
}

void AEvaPlayerController::LogTitleUIState(const FString& Context, const bool bCreateWidgetAttempted,
    const bool bAddToViewportAttempted, const bool bFocusAssigned, const FString& FailureReason) const
{
    TSubclassOf<UEvaTitleMenuWidget> WidgetClass = TitleMenuWidgetClass;
    if (!WidgetClass.Get())
    {
        WidgetClass = UEvaTitleMenuWidget::StaticClass();
    }

    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    const bool bViewportClientValid = LocalPlayer && LocalPlayer->ViewportClient;
    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[TitleUI] Context=%s TitleWidgetClass=%s TitleWidgetClassValid=%s CreateWidgetAttempted=%s CreateWidgetResult=%s WidgetInstanceName=%s RootWidgetValid=%s AddToViewportAttempted=%s IsInViewport=%s Visibility=%s RenderOpacity=%.2f ZOrder=100 NativeConstructCalled=%s FocusAssigned=%s LocalController=%s LocalPlayerValid=%s GameViewportClientValid=%s FailureReason=%s"),
        *Context,
        WidgetClass.Get() ? *WidgetClass->GetName() : TEXT("None"),
        WidgetClass.Get() ? TEXT("true") : TEXT("false"),
        bCreateWidgetAttempted ? TEXT("true") : TEXT("false"),
        TitleMenuWidget ? TEXT("true") : TEXT("false"),
        TitleMenuWidget ? *TitleMenuWidget->GetName() : TEXT("None"),
        TitleMenuWidget && TitleMenuWidget->HasNativeMenuRoot() ? TEXT("true") : TEXT("false"),
        bAddToViewportAttempted ? TEXT("true") : TEXT("false"),
        TitleMenuWidget && TitleMenuWidget->IsInViewport() ? TEXT("true") : TEXT("false"),
        TitleMenuWidget ? *UEnum::GetValueAsString(TitleMenuWidget->GetVisibility()) : TEXT("None"),
        TitleMenuWidget ? TitleMenuWidget->GetRenderOpacity() : 0.0f,
        TitleMenuWidget && TitleMenuWidget->WasNativeConstructCalled() ? TEXT("true") : TEXT("false"),
        bFocusAssigned ? TEXT("true") : TEXT("false"),
        IsLocalController() ? TEXT("true") : TEXT("false"),
        LocalPlayer ? TEXT("true") : TEXT("false"),
        bViewportClientValid ? TEXT("true") : TEXT("false"),
        FailureReason.IsEmpty() ? TEXT("None") : *FailureReason);
}

void AEvaPlayerController::CloseAllMenus()
{
    RemoveWidget(TitleMenuWidget);
    RemoveWidget(PauseMenuWidget);
    RemoveWidget(GameOverWidget);
    RemoveWidget(StageClearWidget);
    RemoveWidget(SettingsWidget);
    TitleMenuWidget = nullptr;
    PauseMenuWidget = nullptr;
    GameOverWidget = nullptr;
    StageClearWidget = nullptr;
    SettingsWidget = nullptr;
}

void AEvaPlayerController::RemoveWidget(UUserWidget* Widget)
{
    if (Widget && Widget->IsInViewport())
    {
        Widget->RemoveFromParent();
    }
}

void AEvaPlayerController::LoadSettings()
{
    SettingsSave = Cast<UEvaSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName, SettingsUserIndex));
    if (!SettingsSave)
    {
        SettingsSave = Cast<UEvaSettingsSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UEvaSettingsSaveGame::StaticClass()));
    }
}

void AEvaPlayerController::SaveAndApplySettings()
{
    ApplySettingsToPlayer();
    if (SettingsSave)
    {
        UGameplayStatics::SaveGameToSlot(SettingsSave, SettingsSlotName, SettingsUserIndex);
    }
    PlayUIClick();
}

void AEvaPlayerController::ApplySettingsToPlayer() const
{
    if (!SettingsSave)
    {
        return;
    }

    if (AEvaPlayerCharacter* EvaPawn = Cast<AEvaPlayerCharacter>(GetPawn()))
    {
        EvaPawn->SetInvertMouseY(SettingsSave->bInvertMouseY);
        EvaPawn->SetMouseSensitivity(SettingsSave->MouseSensitivity);
    }
}

void AEvaPlayerController::SetMasterVolumeSetting(const float Value)
{
    if (SettingsSave)
    {
        SettingsSave->MasterVolume = FMath::Clamp(Value, 0.0f, 1.0f);
    }
}

void AEvaPlayerController::SetBGMVolumeSetting(const float Value)
{
    if (SettingsSave)
    {
        SettingsSave->BGMVolume = FMath::Clamp(Value, 0.0f, 1.0f);
    }
}

void AEvaPlayerController::SetSFXVolumeSetting(const float Value)
{
    if (SettingsSave)
    {
        SettingsSave->SFXVolume = FMath::Clamp(Value, 0.0f, 1.0f);
    }
}

void AEvaPlayerController::SetMouseSensitivitySetting(const float Value)
{
    if (SettingsSave)
    {
        SettingsSave->MouseSensitivity = FMath::Clamp(Value, 0.1f, 5.0f);
        ApplySettingsToPlayer();
    }
}

void AEvaPlayerController::SetInvertMouseYSetting(const bool bInvert)
{
    if (SettingsSave)
    {
        SettingsSave->bInvertMouseY = bInvert;
        ApplySettingsToPlayer();
    }
}

float AEvaPlayerController::GetMasterVolumeSetting() const
{
    return SettingsSave ? SettingsSave->MasterVolume : 0.8f;
}

float AEvaPlayerController::GetBGMVolumeSetting() const
{
    return SettingsSave ? SettingsSave->BGMVolume : 0.45f;
}

float AEvaPlayerController::GetSFXVolumeSetting() const
{
    return SettingsSave ? SettingsSave->SFXVolume : 0.85f;
}

float AEvaPlayerController::GetMouseSensitivitySetting() const
{
    return SettingsSave ? SettingsSave->MouseSensitivity : 1.0f;
}

bool AEvaPlayerController::IsMouseYInvertedSetting() const
{
    return SettingsSave && SettingsSave->bInvertMouseY;
}

void AEvaPlayerController::PlayTone(const float Frequency, const float Duration, const float VolumeScale) const
{
    if (!GetWorld() || Frequency <= 0.0f || Duration <= 0.0f)
    {
        return;
    }

    constexpr int32 SampleRate = 22050;
    constexpr int32 NumChannels = 1;
    const int32 SampleCount = FMath::Max(1, FMath::RoundToInt(SampleRate * Duration));
    TArray<int16> Samples;
    Samples.SetNumZeroed(SampleCount);

    const float Volume = FMath::Clamp(GetMasterVolumeSetting() * GetSFXVolumeSetting() * VolumeScale, 0.0f, 1.0f);
    for (int32 Index = 0; Index < SampleCount; ++Index)
    {
        const float T = static_cast<float>(Index) / static_cast<float>(SampleRate);
        const float Envelope = 1.0f - (static_cast<float>(Index) / static_cast<float>(SampleCount));
        const float Wave = FMath::Sin(2.0f * PI * Frequency * T) * Envelope * Volume;
        Samples[Index] = static_cast<int16>(FMath::Clamp(Wave, -1.0f, 1.0f) * 32767.0f);
    }

    USoundWaveProcedural* Sound = NewObject<USoundWaveProcedural>(GetTransientPackage());
    if (!Sound)
    {
        return;
    }

    Sound->SetSampleRate(SampleRate);
    Sound->NumChannels = NumChannels;
    Sound->Duration = Duration;
    Sound->SoundGroup = SOUNDGROUP_UI;
    Sound->bLooping = false;
    Sound->QueueAudio(reinterpret_cast<uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
    UGameplayStatics::PlaySound2D(GetWorld(), Sound, 1.0f);
}

void AEvaPlayerController::PlayUIClick() const
{
    PlayTone(880.0f, 0.045f, 0.45f);
}

void AEvaPlayerController::PlayUIEvent(const float Frequency, const float Duration, const float VolumeScale) const
{
    PlayTone(Frequency, Duration, VolumeScale);
}
