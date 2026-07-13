#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "EvaPlayerCharacter.generated.h"

class AEvaWeaponBase;
class UCameraComponent;
class UEvaHealthComponent;
class UEvaPlayerTelemetryComponent;
class UInputAction;
class UInputMappingContext;
class UAIPerceptionStimuliSourceComponent;

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEvaPlayerCharacter();

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
    void SpawnStarterWeapon();
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
};
