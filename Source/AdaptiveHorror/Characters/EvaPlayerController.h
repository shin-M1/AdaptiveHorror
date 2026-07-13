#pragma once

#include "CoreMinimal.h"
#include "Core/EvaGameFlowTypes.h"
#include "GameFramework/PlayerController.h"
#include "EvaPlayerController.generated.h"

class UEvaGameOverWidget;
class UEvaPauseMenuWidget;
class UEvaSettingsSaveGame;
class UEvaSettingsWidget;
class UEvaStageClearWidget;
class UEvaTitleMenuWidget;
class UUserWidget;

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void ShowTitleMenu();

    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void ShowPauseMenu();

    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void ShowGameOverMenu();

    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void ShowStageClearMenu();

    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void StartNewGameFromMenu();

    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void ResumeGameFromMenu();

    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void RetryFromCheckpointFromMenu();

    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void ReturnToTitleFromMenu();

    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void ExitGameFromMenu();

    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void OpenSettingsMenu(EEvaSettingsReturnTarget ReturnTarget);

    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void ReturnFromSettingsMenu(EEvaSettingsReturnTarget ReturnTarget);

    UFUNCTION(BlueprintCallable, Category = "EVA|Input")
    void ApplyGameplayInputMode();

    UFUNCTION(BlueprintCallable, Category = "EVA|UI")
    void CloseMenusForGameplay();

    UFUNCTION(BlueprintCallable, Category = "EVA|Input")
    void ApplyMenuInputMode(bool bPauseGameInput = true);

    UFUNCTION(BlueprintCallable, Category = "EVA|Settings")
    void SaveAndApplySettings();

    UFUNCTION(BlueprintCallable, Category = "EVA|Settings")
    void SetMasterVolumeSetting(float Value);

    UFUNCTION(BlueprintCallable, Category = "EVA|Settings")
    void SetBGMVolumeSetting(float Value);

    UFUNCTION(BlueprintCallable, Category = "EVA|Settings")
    void SetSFXVolumeSetting(float Value);

    UFUNCTION(BlueprintCallable, Category = "EVA|Settings")
    void SetMouseSensitivitySetting(float Value);

    UFUNCTION(BlueprintCallable, Category = "EVA|Settings")
    void SetInvertMouseYSetting(bool bInvert);

    UFUNCTION(BlueprintPure, Category = "EVA|Settings")
    float GetMasterVolumeSetting() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Settings")
    float GetBGMVolumeSetting() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Settings")
    float GetSFXVolumeSetting() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Settings")
    float GetMouseSensitivitySetting() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Settings")
    bool IsMouseYInvertedSetting() const;

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|UI")
    TSubclassOf<UEvaTitleMenuWidget> TitleMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|UI")
    TSubclassOf<UEvaPauseMenuWidget> PauseMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|UI")
    TSubclassOf<UEvaGameOverWidget> GameOverWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|UI")
    TSubclassOf<UEvaStageClearWidget> StageClearWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|UI")
    TSubclassOf<UEvaSettingsWidget> SettingsWidgetClass;

private:
    void HandleEscapePressed();
    void CloseAllMenus();
    void RemoveWidget(UUserWidget* Widget);
    void LoadSettings();
    void ApplySettingsToPlayer() const;
    void PlayTone(float Frequency, float Duration, float VolumeScale) const;
    void PlayUIClick() const;
    void PlayUIEvent(float Frequency, float Duration, float VolumeScale) const;

    UPROPERTY()
    TObjectPtr<UEvaTitleMenuWidget> TitleMenuWidget;

    UPROPERTY()
    TObjectPtr<UEvaPauseMenuWidget> PauseMenuWidget;

    UPROPERTY()
    TObjectPtr<UEvaGameOverWidget> GameOverWidget;

    UPROPERTY()
    TObjectPtr<UEvaStageClearWidget> StageClearWidget;

    UPROPERTY()
    TObjectPtr<UEvaSettingsWidget> SettingsWidget;

    UPROPERTY()
    TObjectPtr<UEvaSettingsSaveGame> SettingsSave;

    static const FString SettingsSlotName;
    static constexpr int32 SettingsUserIndex = 0;
};
