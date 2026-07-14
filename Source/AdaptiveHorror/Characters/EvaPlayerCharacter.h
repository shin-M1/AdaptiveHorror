#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "EvaPlayerCharacter.generated.h"

class AEvaWeaponBase;
class AEvaFacilityInteractable;
class UCameraComponent;
class UEvaHealthComponent;
class UEvaPlayerTelemetryComponent;
class UInputAction;
class UInputMappingContext;
class UAIPerceptionStimuliSourceComponent;
class USpotLightComponent;

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEvaPlayerCharacter();
    virtual void Tick(float DeltaSeconds) override;

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintPure, Category = "EVA|Player")
    UEvaHealthComponent* GetHealthComponent() const { return HealthComponent; }

    UFUNCTION(BlueprintPure, Category = "EVA|Player")
    UEvaPlayerTelemetryComponent* GetTelemetryComponent() const { return TelemetryComponent; }

    UFUNCTION(BlueprintPure, Category = "EVA|Player")
    AEvaWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

    UFUNCTION(BlueprintPure, Category = "EVA|Input")
    bool IsMouseYInverted() const { return bInvertMouseY; }

    UFUNCTION(BlueprintCallable, Category = "EVA|Input")
    void SetInvertMouseY(bool bNewInvertMouseY) { bInvertMouseY = bNewInvertMouseY; }

    UFUNCTION(BlueprintPure, Category = "EVA|Input")
    float GetMouseSensitivity() const { return MouseSensitivity; }

    UFUNCTION(BlueprintCallable, Category = "EVA|Input")
    void SetMouseSensitivity(float NewMouseSensitivity);

    UFUNCTION(BlueprintPure, Category = "EVA|Player")
    bool IsDead() const;

    UFUNCTION(BlueprintCallable, Category = "EVA|Player")
    void ResetForCheckpoint(const FTransform& CheckpointTransform);

    UFUNCTION(BlueprintCallable, Category = "EVA|Horror")
    void TriggerCameraShakeFeedback(float Intensity = 0.35f, float Duration = 0.35f);

    UFUNCTION(BlueprintCallable, Category = "EVA|Horror")
    void TriggerDamageFeedback(float DamageAmount);

    UFUNCTION(BlueprintPure, Category = "EVA|Horror")
    float GetDamageFeedbackIntensity() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Horror")
    float GetLowHealthVignetteIntensity() const;

    UFUNCTION(BlueprintCallable, Category = "EVA|Light")
    void SetFlashlightEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "EVA|Light")
    void ToggleFlashlight();

    UFUNCTION(BlueprintPure, Category = "EVA|Light")
    bool IsFlashlightEnabled() const { return bFlashlightEnabled; }

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction")
    FString GetInteractionPrompt() const { return FocusedInteractionPrompt; }

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction|Debug")
    FString GetFocusedInteractableDebugName() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction|Debug")
    FString GetLastInteractionFailure() const { return LastInteractionFailure; }

protected:
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;
    virtual void PawnClientRestart() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void BuildRuntimeInputMapping();
    void AddRuntimeInputMapping();
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartSprint();
    void StopSprint();
    void StartJump();
    void StopJump();
    void FireWeapon();
    void ReloadWeapon();
    void ToggleFlashlightInput();
    void Interact();
    void SpawnStarterWeapon();
    void UpdateFocusedInteractable(bool bLogDiagnostics = false, bool bInputReceived = false);
    void LogInteractionDiagnostics(const FString& Context, bool bInputReceived, bool bExecuteResult) const;
    void UpdateFlashlightVisibility();
    void ResetHorrorFeedback();
    void PlayBreathingPulse();
    void DebugIncreaseEvaAnalysis();
    void DebugForceHunterSpawn();
    void DebugForceZombieWave();
    void DebugWarpToAdamArena();
    void DebugRestorePlayer();
    void DebugForceStageClear();
    void DebugPrintTelemetrySnapshot();
    void DebugToggleNavigationVisualization();
    bool IsStageClearActive() const;
    bool IsGameplayInputAllowed() const;
    bool IsLookInputAllowed() const;

    UFUNCTION()
    void HandleDeath(AActor* DeadActor);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Player")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Light")
    TObjectPtr<USpotLightComponent> FlashlightComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Player")
    TObjectPtr<UEvaHealthComponent> HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Player")
    TObjectPtr<UEvaPlayerTelemetryComponent> TelemetryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Player")
    TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSource;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Player")
    float WalkSpeed = 450.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Player")
    float SprintSpeed = 750.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Input")
    bool bInvertMouseY = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Input", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float MouseSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Weapon")
    TSubclassOf<AEvaWeaponBase> StarterWeaponClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Weapon")
    TObjectPtr<AEvaWeaponBase> CurrentWeapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Light")
    bool bFlashlightEnabled = true;

private:
    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> RuntimeMappingContext;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> FireAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> ReloadAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> FlashlightAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> DebugIncreaseAnalysisAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> DebugForceHunterAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> DebugForceZombieWaveAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> DebugWarpAdamAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> DebugRestoreAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> DebugForceClearAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> DebugTelemetrySnapshotAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> DebugNavigationVisualizationAction;

    FName LastDamageCause = TEXT("Unknown");
    bool bIsSprinting = false;
    FVector BaseCameraRelativeLocation = FVector::ZeroVector;
    FRotator BaseCameraRelativeRotation = FRotator::ZeroRotator;
    float CameraShakeEndTime = -1000.0f;
    float CameraShakeIntensity = 0.0f;
    float LastDamageFeedbackTime = -1000.0f;
    float DamageFeedbackDuration = 0.75f;
    float LastDamageFeedbackScale = 0.0f;
    FTimerHandle BreathingTimer;
    TWeakObjectPtr<AEvaFacilityInteractable> FocusedInteractable;
    FString FocusedInteractionPrompt;
    float InteractionTraceDistance = 360.0f;
    FString LastInteractionFailure = TEXT("None");
    FString LastInteractionHitActor = TEXT("None");
    FString LastInteractionHitComponent = TEXT("None");
    FVector LastInteractionCameraLocation = FVector::ZeroVector;
    FVector LastInteractionTraceStart = FVector::ZeroVector;
    FVector LastInteractionTraceEnd = FVector::ZeroVector;
    float LastInteractionDistance = -1.0f;
    bool bLastInteractionHit = false;
    bool bLastInteractionImplementsInteractable = false;
    bool bLastInteractionEnabled = false;
    bool bLastInteractionLineOfSightClear = false;
};
