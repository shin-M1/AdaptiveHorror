#pragma once

#include "CoreMinimal.h"
#include "AI/EvaTelemetryTypes.h"
#include "GameFramework/Actor.h"
#include "EvaResearchFacilityDirector.generated.h"

class AEvaAdamBossCharacter;
class AEvaHunterCharacter;
class AEvaZombieCharacter;
class AEvaAmmoPickup;
class AEvaHealthPickup;

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaResearchFacilityDirector : public AActor
{
    GENERATED_BODY()

public:
    AEvaResearchFacilityDirector();

    UFUNCTION(BlueprintCallable, Category = "EVA|Facility")
    void NotifyZoneEntered(EEvaFacilityZone NewZone);

    UFUNCTION(BlueprintCallable, Category = "EVA|Facility")
    void NotifyStoryLogCollected(FName LogId, const FString& Title, const FString& Body);

    UFUNCTION(BlueprintCallable, Category = "EVA|Facility")
    void NotifyAdamDefeated(AEvaAdamBossCharacter* Adam);

    UFUNCTION(BlueprintCallable, Category = "EVA|Facility")
    void StartAdamEncounter();

    UFUNCTION(BlueprintCallable, Category = "EVA|Facility")
    void CompleteStage();

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    EEvaFacilityZone GetCurrentZone() const { return CurrentZone; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    FString GetCurrentZoneName() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    FString GetObjectiveText() const { return CurrentObjective; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    int32 GetCollectedStoryLogCount() const { return CollectedStoryLogs.Num(); }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    bool HasCollectedStoryLog(FName LogId) const { return CollectedStoryLogs.Contains(LogId); }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    bool HasAnyEvaLog() const { return bEvaLogAcquired; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    bool IsStageClear() const { return bStageClear; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    bool IsAdamEncounterActive() const { return bAdamEncounterActive; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    bool IsEvolutionUnlocked() const { return bEvolutionUnlocked; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    bool ShouldDisplayStoryLog() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    FString GetLastStoryLogTitle() const { return LastStoryLogTitle; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    FString GetLastStoryLogBody() const { return LastStoryLogBody; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    FTransform GetHunterSpawnTransform() const { return HunterSpawnTransform; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    FTransform GetAdamSpawnTransform() const { return AdamSpawnTransform; }

    UFUNCTION(BlueprintPure, Category = "EVA|Facility")
    AEvaAdamBossCharacter* GetActiveAdam() const { return ActiveAdam; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Facility")
    TSubclassOf<AEvaZombieCharacter> ZombieClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Facility")
    TSubclassOf<AEvaAdamBossCharacter> AdamBossClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Facility")
    TSubclassOf<AEvaAmmoPickup> AmmoPickupClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Facility")
    TSubclassOf<AEvaHealthPickup> HealthPickupClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Facility")
    float StoryLogDisplaySeconds = 8.0f;

private:
    void SpawnZoneEncounter(EEvaFacilityZone Zone);
    AEvaZombieCharacter* SpawnZombieAt(const FVector& Location, EEvaEvolutionType EvolutionType = EEvaEvolutionType::None,
        const FString& SpawnReason = TEXT("ZoneEncounter"));
    void SpawnSupportPickupsForZone(EEvaFacilityZone Zone);
    void SetObjectiveForZone(EEvaFacilityZone Zone);
    AEvaAdamBossCharacter* FindExistingLivingAdam() const;
    int32 CountExistingLivingAdam() const;
    void LogAdamEncounterState(const FString& Context, bool bSpawnAttempted, AEvaAdamBossCharacter* SpawnResult,
        const FString& DestroyReason = TEXT("")) const;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Facility")
    EEvaFacilityZone CurrentZone = EEvaFacilityZone::EntryLobby;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Facility")
    bool bEvaLogAcquired = false;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Facility")
    bool bHunterEventTriggered = false;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Facility")
    bool bEvolutionUnlocked = false;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Facility")
    bool bAdamEncounterActive = false;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Facility")
    bool bStageClear = false;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Facility")
    FString CurrentObjective = TEXT("Enter the facility.");

    UPROPERTY(VisibleAnywhere, Category = "EVA|Facility")
    TArray<EEvaFacilityZone> TriggeredZones;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Facility")
    TArray<FName> CollectedStoryLogs;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Facility")
    FString LastStoryLogTitle;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Facility")
    FString LastStoryLogBody;

    float LastStoryLogTime = -1000.0f;

    UPROPERTY()
    TObjectPtr<AEvaAdamBossCharacter> ActiveAdam;

    FTransform HunterSpawnTransform;
    FTransform AdamSpawnTransform;
};
