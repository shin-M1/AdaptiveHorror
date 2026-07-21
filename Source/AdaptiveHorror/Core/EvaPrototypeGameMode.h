#pragma once

#include "CoreMinimal.h"
#include "AI/EvaTelemetryTypes.h"
#include "Core/EvaGameFlowTypes.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "EvaPrototypeGameMode.generated.h"

class AEvaPlayerCharacter;
class AEvaHunterCharacter;
class AEvaAdamBossCharacter;
class AEvaFacilityInteractable;
class AEvaResearchFacilityDirector;
class AEvaZombieCharacter;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UPrimitiveComponent;
class UPointLightComponent;
class USkyLightComponent;
class UStaticMesh;
enum class EEvaFacilityInteractableType : uint8;

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaPrototypeGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AEvaPrototypeGameMode();

    UFUNCTION(BlueprintCallable, Category = "EVA|Game")
    void HandlePlayerDeath(AEvaPlayerCharacter* DeadPlayer);

    UFUNCTION(BlueprintCallable, Category = "EVA|Checkpoint")
    void ActivateCheckpoint(const FTransform& CheckpointTransform);

    UFUNCTION(BlueprintPure, Category = "EVA|Game")
    bool IsGameOver() const { return bGameOver; }

    UFUNCTION(BlueprintPure, Category = "EVA|Checkpoint")
    FTransform GetLastCheckpointTransform() const { return LastCheckpointTransform; }

    UFUNCTION(BlueprintCallable, Category = "EVA|AI")
    void NotifyEnemyKilled(AEvaZombieCharacter* DeadEnemy);

    UFUNCTION(BlueprintCallable, Category = "EVA|Hunter")
    void NotifyHunterDefeated(AEvaHunterCharacter* DefeatedHunter);

    UFUNCTION(BlueprintCallable, Category = "EVA|Hunter")
    void SpawnHunter();

    UFUNCTION(BlueprintCallable, Category = "EVA|Game")
    void HandleStageClear();

    UFUNCTION(BlueprintCallable, Category = "EVA|Game")
    void EnterTitleMode();

    UFUNCTION(BlueprintCallable, Category = "EVA|Game")
    void StartNewGameFlow();

    UFUNCTION(BlueprintCallable, Category = "EVA|Game")
    void PauseGameFlow();

    UFUNCTION(BlueprintCallable, Category = "EVA|Game")
    void ResumeGameFlow();

    UFUNCTION(BlueprintCallable, Category = "EVA|Game")
    void RetryFromCheckpointFlow();

    UFUNCTION(BlueprintCallable, Category = "EVA|Game")
    void ReturnToTitleFlow();

    UFUNCTION(BlueprintPure, Category = "EVA|Game")
    EEvaGameFlowState GetGameFlowState() const { return GameFlowState; }

    UFUNCTION(BlueprintPure, Category = "EVA|Game")
    bool IsGameplayActive() const { return GameFlowState == EEvaGameFlowState::Playing && !bGameOver && !bStageClear; }

    UFUNCTION(BlueprintPure, Category = "EVA|Game")
    bool CanPlayerTakeDamage() const { return IsGameplayActive(); }

    UFUNCTION(BlueprintPure, Category = "EVA|Game")
    bool IsStageClear() const { return bStageClear; }

    UFUNCTION(BlueprintPure, Category = "EVA|Hunter")
    int32 GetHunterDefeatCount() const { return HunterDefeatCount; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    AEvaResearchFacilityDirector* GetResearchDirector() const { return CurrentDirector; }

    UFUNCTION(BlueprintCallable, Category = "EVA|Facility")
    void SetFacilityPowerOnline(bool bOnline);

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    bool IsFacilityPowerOnlineForDebug() const { return bFacilityPowerOnline; }

    UFUNCTION(BlueprintCallable, Category = "EVA|Debug")
    void DebugIncreaseEvaAnalysis(float Amount = 20.0f);

    UFUNCTION(BlueprintCallable, Category = "EVA|Debug")
    void DebugForceHunterSpawn();

    UFUNCTION(BlueprintCallable, Category = "EVA|Debug")
    void DebugForceZombieWave();

    UFUNCTION(BlueprintCallable, Category = "EVA|Debug")
    void DebugWarpPlayerToAdamArena();

    UFUNCTION(BlueprintCallable, Category = "EVA|Debug")
    void DebugRestorePlayer();

    UFUNCTION(BlueprintCallable, Category = "EVA|Debug")
    void DebugForceStageClear();

    UFUNCTION(BlueprintCallable, Category = "EVA|Debug")
    void DebugPrintTelemetrySnapshot();

    UFUNCTION(BlueprintCallable, Category = "EVA|Debug")
    void DebugToggleNavigationVisualization();

    UFUNCTION(BlueprintCallable, Category = "EVA|Debug")
    void ShowDebugStatusMessage(const FString& Message, float Duration = 4.0f);

    bool FindSafeEnemySpawnLocation(const FVector& Origin, float MinRadius, float MaxRadius,
        float MinEnemySeparation, float MinPlayerDistance, FVector& OutLocation,
        bool bAvoidPlayerView = true) const;

    AEvaZombieCharacter* SpawnEnemyNearLocation(TSubclassOf<AEvaZombieCharacter> EnemyClass,
        const FVector& Origin, float MinRadius, float MaxRadius, const FString& EnemyType,
        const FString& SpawnReason, EEvaEvolutionType EvolutionType = EEvaEvolutionType::None);

    void NotifyFallbackMovementUsed();
    void NotifyEnemyStuck(const FString& EnemyName);

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    FString GetDebugStatusMessage() const { return LastDebugStatusMessage; }

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    bool ShouldDisplayDebugStatusMessage() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    int32 GetActiveZombieCount() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    int32 GetActiveHunterCount() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    int32 GetActiveAdamCount() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    FString GetLastSpawnResult() const { return LastSpawnResult; }

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    FVector GetLastSpawnLocation() const { return LastSpawnLocation; }

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    bool IsNavMeshAvailable() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    int32 GetFallbackMovementCount() const { return FallbackMovementCount; }

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    int32 GetStuckEnemyCount() const { return StuckEnemyCount; }

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    bool IsNavigationDebugVisible() const { return bNavigationDebugVisible; }

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    bool IsDebugHUDVisible() const { return bDebugHUDVisible; }

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    int32 GetDebugHUDPageIndex() const { return DebugHUDPageIndex; }

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    int32 GetDebugHUDPageCount() const { return DebugHUDPageCount; }

    UFUNCTION(BlueprintPure, Category = "EVA|Debug")
    bool IsRespawnScheduledForDebug() const;

    UFUNCTION(BlueprintCallable, Category = "EVA|Horror")
    void TriggerBlackout(float Duration = 3.5f, bool bForce = false);

    UFUNCTION(BlueprintCallable, Category = "EVA|Horror")
    void TriggerHunterArrivalEffect(const FVector& ArrivalLocation);

    UFUNCTION(BlueprintCallable, Category = "EVA|Horror")
    void TriggerAdamEntranceEffect(AEvaAdamBossCharacter* Adam);

    UFUNCTION(BlueprintCallable, Category = "EVA|Horror")
    void TriggerAdamChargeEffect(AEvaAdamBossCharacter* Adam);

    UFUNCTION(BlueprintCallable, Category = "EVA|Horror")
    void TriggerAdamRoarEffect(AEvaAdamBossCharacter* Adam);

    UFUNCTION(BlueprintCallable, Category = "EVA|Horror")
    void TriggerAdamPhaseTwoEffect(AEvaAdamBossCharacter* Adam);

    UFUNCTION(BlueprintCallable, Category = "EVA|Horror")
    void TriggerDoorEffect(const FVector& Location, const FString& DoorLabel);

    UFUNCTION(BlueprintCallable, Category = "EVA|Horror")
    void TriggerPlayerDamageEffect(float DamageAmount);

    UFUNCTION(BlueprintPure, Category = "EVA|Horror")
    bool IsBlackoutActive() const { return bBlackoutActive; }

    UFUNCTION(BlueprintPure, Category = "EVA|Horror")
    float GetBlackoutOverlayIntensity() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Horror")
    float GetHorrorPulseIntensity() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Horror")
    bool ShouldDisplayHorrorWarning() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Horror")
    FString GetHorrorWarningText() const { return LastHorrorWarningText; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Prototype")
    bool bBuildRuntimeArena = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Prototype")
    float RespawnDelay = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Prototype")
    float AdaptiveSpawnInterval = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter")
    int32 HunterSpawnKillThreshold = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter")
    float HunterTimeSpawnDelay = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter")
    float HunterReinsertDelay = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Prototype")
    int32 MaxLivingEnemies = 6;

private:
    void BuildPrototypeArena();
    void BuildRuntimeNavigation();
    void LogNavigationStatus(const FString& Context) const;
    void CheckRuntimeNavigationReady();
    void EnsurePrototypePlayer();
    void SpawnPrototypeEnemies();
    void SpawnAdaptiveEnemy();
    void SpawnInitialZombie();
    void RespawnPlayer();
    FVector PickEnemySpawnLocation() const;
    int32 CountLivingEnemies() const;
    AActor* SpawnArenaBox(const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator);
    AActor* SpawnTaggedArenaBox(const FVector& Location, const FVector& Scale, const FName Tag,
        const FRotator& Rotation = FRotator::ZeroRotator);
    void BuildFacilityZone(const FVector& Center, const FString& Label, const int32 ZoneIndex);
    void BuildZoneIdentityGeometry(const FVector& Center, int32 ZoneIndex, float SideWallY,
        int32& OutObstacleCount, int32& OutLandmarkCount, FString& OutZoneShape, float& OutAverageHeight);
    void LogZoneIdentity(const FString& Label, int32 ZoneIndex, const FString& ZoneShape, const FVector& FloorScale,
        int32 ObstacleCount, int32 LandmarkCount, float AverageHeight) const;
    void RegisterRuntimeFloorComponent(UPrimitiveComponent* FloorComponent);
    FBox CalculateRuntimeFacilityBounds() const;
    bool ValidateNavigationProjection(const FString& Context, const FVector& Location, FVector* OutProjectedLocation = nullptr) const;
    void LogRuntimeClassBindings() const;
    void StartCombatSpawningAfterNavigationReady();
    void SpawnFacilityTrigger(AEvaResearchFacilityDirector* Director, EEvaFacilityZone Zone, const FVector& Location);
    AEvaFacilityInteractable* SpawnFacilityInteractable(AEvaResearchFacilityDirector* Director,
        const FVector& Location, const FRotator& Rotation, EEvaFacilityInteractableType Type,
        const FString& DisplayName, FName LogId = NAME_None, const FString& LogTitle = TEXT(""),
        const FString& LogBody = TEXT(""));
    void LogFacilityInteractableSpawnStatus(const FString& Context) const;
    void SpawnStoryLog(AEvaResearchFacilityDirector* Director, FName LogId, const FString& Title,
        const FString& Body, const FVector& Location);
    void ResetEnemyTargets();
    int32 StopAllEnemyCombatForStageClear();
    void ClearStageClearTimers();
    void CleanupCombatActorsForFlowReset();
    void SetGameFlowState(EEvaGameFlowState NewState);
    void LogStageClearState(const FString& Context, int32 ClearedEnemyAI, bool bClearedTimers) const;
    void LogPlayerDeathRequest(const FString& Context, const AEvaPlayerCharacter* DeadPlayer,
        bool bRespawnTimerCreated) const;
    void PrimeEnemyForPlayer(AEvaZombieCharacter* Enemy) const;
    int32 CleanupAdamArenaDebugEnemies(const FVector& ArenaLocation, float Radius);
    bool CanRunHorrorEffect(bool bAllowDuringPaused = false) const;
    void BeginHorrorRuntimeEffects();
    void StopHorrorRuntimeEffects(bool bRestoreLighting = true);
    void UpdateEmergencyLightFlicker();
    void RestoreHorrorLighting();
    void EndBlackout();
    void PlayAmbientPulse();
    void SetHorrorWarning(const FString& Message, float Duration);
    void SpawnRuntimeFog();
    void StartAdaptationProfileUpdates();
    void StopAdaptationProfileUpdates();
    void UpdateAdaptationProfileForGameplay();
    void SyncEnemyDebugIntentDisplays(bool bForceLog) const;

    UPROPERTY()
    TObjectPtr<UStaticMesh> RuntimeCubeMesh;

    UPROPERTY()
    TObjectPtr<class ANavMeshBoundsVolume> RuntimeNavBoundsVolume;

    UPROPERTY()
    TObjectPtr<AEvaPlayerCharacter> PlayerAwaitingRespawn;

    FTransform LastCheckpointTransform;
    FTimerHandle RespawnTimer;
    FTimerHandle EnemySpawnTimer;
    FTimerHandle AdaptiveSpawnTimer;
    FTimerHandle HunterTimeSpawnTimer;
    FTimerHandle HunterReinsertTimer;
    FTimerHandle NavigationReadinessTimer;
    FTimerHandle AdaptationProfileTimer;
    bool bGameOver = false;
    EEvaGameFlowState GameFlowState = EEvaGameFlowState::Loading;

    UPROPERTY()
    TObjectPtr<AEvaHunterCharacter> CurrentHunter;

    UPROPERTY()
    TObjectPtr<AEvaResearchFacilityDirector> CurrentDirector;

    int32 TotalZombieKills = 0;
    int32 HunterDefeatCount = 0;
    int32 HunterTierToSpawn = 1;
    bool bStageClear = false;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Debug")
    FString LastDebugStatusMessage;

    float LastDebugStatusMessageTime = -1000.0f;
    float DebugStatusMessageDuration = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Debug")
    FString LastSpawnResult = TEXT("No enemy spawn attempted.");

    UPROPERTY(VisibleAnywhere, Category = "EVA|Debug")
    FVector LastSpawnLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Debug")
    int32 FallbackMovementCount = 0;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Debug")
    int32 StuckEnemyCount = 0;

    bool bInitialZombieSpawned = false;
    bool bNavigationDebugVisible = false;
    bool bDebugHUDVisible = false;
    int32 DebugHUDPageIndex = 0;
    static constexpr int32 DebugHUDPageCount = 3;
    bool bRuntimeNavigationReady = false;
    bool bRuntimeNavigationFailed = false;
    int32 NavigationReadinessAttempts = 0;
    FBox RuntimeFacilityBounds = FBox(EForceInit::ForceInit);
    TSet<FName> SpawnedFacilityInteractableKeys;

    UPROPERTY()
    TArray<TObjectPtr<UPrimitiveComponent>> RuntimeFloorComponents;

    UPROPERTY()
    TObjectPtr<UDirectionalLightComponent> RuntimeDirectionalLightComponent;

    UPROPERTY()
    TObjectPtr<USkyLightComponent> RuntimeSkyLightComponent;

    UPROPERTY()
    TObjectPtr<UPointLightComponent> RuntimeMainPointLightComponent;

    UPROPERTY()
    TArray<TObjectPtr<UPointLightComponent>> RuntimeEmergencyLightComponents;

    UPROPERTY()
    TObjectPtr<UExponentialHeightFogComponent> RuntimeFogComponent;

    TArray<float> RuntimeEmergencyLightBaseIntensities;
    float RuntimeDirectionalLightBaseIntensity = 1.35f;
    float RuntimeSkyLightBaseIntensity = 0.28f;
    float RuntimeMainPointLightBaseIntensity = 4200.0f;
    bool bFacilityPowerOnline = false;
    float AdaptationProfileUpdateInterval = 4.0f;
    FTimerHandle EmergencyLightFlickerTimer;
    FTimerHandle BlackoutTimer;
    FTimerHandle AmbientPulseTimer;
    bool bBlackoutActive = false;
    float BlackoutEndTime = -1000.0f;
    float HorrorPulseEndTime = -1000.0f;
    float LastHorrorWarningTime = -1000.0f;
    float HorrorWarningDuration = 0.0f;
    FString LastHorrorWarningText;

    UPROPERTY()
    TWeakObjectPtr<AEvaAdamBossCharacter> LastAdamEntranceEffectActor;
};
