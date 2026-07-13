#include "Characters/EvaPlayerController.h"
#include "AdaptiveHorror.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Core/EvaSettingsSaveGame.h"
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

void AEvaPlayerController::ShowTitleMenu()
{
    CloseAllMenus();
    TSubclassOf<UEvaTitleMenuWidget> WidgetClass = TitleMenuWidgetClass;
    if (!WidgetClass.Get())
    {
        WidgetClass = UEvaTitleMenuWidget::StaticClass();
    }
    TitleMenuWidget = CreateWidget<UEvaTitleMenuWidget>(this, WidgetClass);
    if (TitleMenuWidget)
    {
        TitleMenuWidget->AddToViewport(100);
    }
    ApplyMenuInputMode(false);
    PlayUIEvent(146.8f, 0.18f, 0.30f);
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
    ApplyMenuInputMode(ReturnTarget == EEvaSettingsReturnTarget::Pause);
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
    bShowMouseCursor = false;
    ResetIgnoreMoveInput();
    ResetIgnoreLookInput();
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
    bShowMouseCursor = true;
    SetIgnoreMoveInput(bPauseGameInput);
    SetIgnoreLookInput(bPauseGameInput);
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
