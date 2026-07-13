#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/EvaGameFlowTypes.h"
#include "Widgets/SWidget.h"
#include "EvaMenuWidgets.generated.h"

class AEvaPlayerController;
class UButton;
class UCheckBox;
class USlider;
class UTextBlock;
class UVerticalBox;

UCLASS(Abstract, Blueprintable)
class ADAPTIVEHORROR_API UEvaMenuWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "EVA|UI")
    bool HasNativeMenuRoot() const;

    UFUNCTION(BlueprintPure, Category = "EVA|UI")
    bool WasNativeConstructCalled() const { return bNativeConstructCalled; }

    bool AssignInitialFocus();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

    virtual FText GetMenuTitleText() const { return FText::GetEmpty(); }
    virtual FText GetMenuSubtitleText() const { return FText::GetEmpty(); }
    virtual void BuildMenuContent(UVerticalBox* RootBox) {}

    UVerticalBox* BuildMenuRoot(const FText& Title, const FText& Subtitle);
    UTextBlock* AddMenuText(UVerticalBox* RootBox, const FText& Text, float FontSize,
        const FLinearColor& Color = FLinearColor::White);
    UButton* AddMenuButton(UVerticalBox* RootBox, const FText& Label, bool bEnabled = true);
    AEvaPlayerController* GetEvaPlayerController() const;
    void SetInitialFocusButton(UButton* Button);

private:
    UPROPERTY()
    TObjectPtr<UButton> InitialFocusButton;

    bool bNativeConstructCalled = false;
};

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API UEvaTitleMenuWidget : public UEvaMenuWidgetBase
{
    GENERATED_BODY()

protected:
    virtual FText GetMenuTitleText() const override;
    virtual FText GetMenuSubtitleText() const override;
    virtual void BuildMenuContent(UVerticalBox* RootBox) override;

private:
    UFUNCTION()
    void HandleNewGameClicked();

    UFUNCTION()
    void HandleSettingsClicked();

    UFUNCTION()
    void HandleExitClicked();
};

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API UEvaPauseMenuWidget : public UEvaMenuWidgetBase
{
    GENERATED_BODY()

protected:
    virtual FText GetMenuTitleText() const override;
    virtual FText GetMenuSubtitleText() const override;
    virtual void BuildMenuContent(UVerticalBox* RootBox) override;

private:
    UFUNCTION()
    void HandleResumeClicked();

    UFUNCTION()
    void HandleRestartCheckpointClicked();

    UFUNCTION()
    void HandleSettingsClicked();

    UFUNCTION()
    void HandleReturnToTitleClicked();

    UFUNCTION()
    void HandleExitClicked();
};

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API UEvaGameOverWidget : public UEvaMenuWidgetBase
{
    GENERATED_BODY()

protected:
    virtual FText GetMenuTitleText() const override;
    virtual FText GetMenuSubtitleText() const override;
    virtual void BuildMenuContent(UVerticalBox* RootBox) override;

private:
    UFUNCTION()
    void HandleRetryClicked();

    UFUNCTION()
    void HandleRestartClicked();

    UFUNCTION()
    void HandleReturnToTitleClicked();
};

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API UEvaStageClearWidget : public UEvaMenuWidgetBase
{
    GENERATED_BODY()

protected:
    virtual FText GetMenuTitleText() const override;
    virtual FText GetMenuSubtitleText() const override;
    virtual void BuildMenuContent(UVerticalBox* RootBox) override;

private:
    UFUNCTION()
    void HandleRetryClicked();

    UFUNCTION()
    void HandleReturnToTitleClicked();

    UFUNCTION()
    void HandleExitClicked();
};

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API UEvaSettingsWidget : public UEvaMenuWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EVA|Settings")
    void SetReturnTarget(EEvaSettingsReturnTarget NewReturnTarget) { ReturnTarget = NewReturnTarget; }

protected:
    virtual FText GetMenuTitleText() const override;
    virtual FText GetMenuSubtitleText() const override;
    virtual void BuildMenuContent(UVerticalBox* RootBox) override;

private:
    UTextBlock* AddLabeledSlider(UVerticalBox* RootBox, const FString& Label, float Value,
        FLinearColor Color, TObjectPtr<USlider>& OutSlider);
    void RefreshValueText();

    UFUNCTION()
    void HandleMasterVolumeChanged(float Value);

    UFUNCTION()
    void HandleBGMVolumeChanged(float Value);

    UFUNCTION()
    void HandleSFXVolumeChanged(float Value);

    UFUNCTION()
    void HandleMouseSensitivityChanged(float Value);

    UFUNCTION()
    void HandleInvertYChanged(bool bIsChecked);

    UFUNCTION()
    void HandleApplyClicked();

    UFUNCTION()
    void HandleBackClicked();

    UPROPERTY()
    TObjectPtr<UTextBlock> ValuesText;

    UPROPERTY()
    TObjectPtr<USlider> MasterVolumeSlider;

    UPROPERTY()
    TObjectPtr<USlider> BGMVolumeSlider;

    UPROPERTY()
    TObjectPtr<USlider> SFXVolumeSlider;

    UPROPERTY()
    TObjectPtr<USlider> MouseSensitivitySlider;

    UPROPERTY()
    TObjectPtr<UCheckBox> InvertYCheckBox;

    EEvaSettingsReturnTarget ReturnTarget = EEvaSettingsReturnTarget::Title;
};
