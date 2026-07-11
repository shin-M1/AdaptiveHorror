#pragma once

#include "CoreMinimal.h"
#include "AI/EvaTelemetryTypes.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "EvaPrototypeGameMode.generated.h"

class AEvaPlayerCharacter;
class AEvaHunterCharacter;
class AEvaResearchFacilityDirector;
class AEvaZombieCharacter;
class UPrimitiveComponent;
class UStaticMesh;

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

    UFUNCTION(BlueprintPure, Category = "EVA|Game")
    bool IsStageClear() const { return bStageClear; }

    UFUNCTION(BlueprintPure, Category = "EVA|Hunter")
    int32 GetHunterDefeatCount() const { return HunterDefeatCount; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    AEvaResearchFacilityDirector* GetResearchDirector() const { return CurrentDirector; }

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
        float MinEnemySeparation, float MinPlayerDistance, FVector& OutLocation) const;

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

protected:
    virtual void BeginPlay() override;

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
    void RegisterRuntimeFloorComponent(UPrimitiveComponent* FloorComponent);
    FBox CalculateRuntimeFacilityBounds() const;
    bool ValidateNavigationProjection(const FString& Context, const FVector& Location, FVector* OutProjectedLocation = nullptr) const;
    void LogRuntimeClassBindings() const;
    void StartCombatSpawningAfterNavigationReady();
    void SpawnFacilityTrigger(AEvaResearchFacilityDirector* Director, EEvaFacilityZone Zone, const FVector& Location);
    void SpawnStoryLog(AEvaResearchFacilityDirector* Director, FName LogId, const FString& Title,
        const FString& Body, const FVector& Location);
    void ResetEnemyTargets();
    void PrimeEnemyForPlayer(AEvaZombieCharacter* Enemy) const;

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
    bool bGameOver = false;

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
    bool bRuntimeNavigationReady = false;
    bool bRuntimeNavigationFailed = false;
    int32 NavigationReadinessAttempts = 0;
    FBox RuntimeFacilityBounds = FBox(EForceInit::ForceInit);

    UPROPERTY()
    TArray<TObjectPtr<UPrimitiveComponent>> RuntimeFloorComponents;
};
