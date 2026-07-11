#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AI/EvaTelemetryTypes.h"
#include "EvaLearningSubsystem.generated.h"

UCLASS(BlueprintType)
class ADAPTIVEHORROR_API UEvaLearningSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EVA|Learning")
    void RecordShot(FName WeaponName);

    UFUNCTION(BlueprintCallable, Category = "EVA|Learning")
    void RecordHit(bool bHeadshot, float CombatDistance);

    UFUNCTION(BlueprintCallable, Category = "EVA|Learning")
    void RecordKill();

    UFUNCTION(BlueprintCallable, Category = "EVA|Learning")
    void RecordDeathCause(FName Cause);

    UFUNCTION(BlueprintCallable, Category = "EVA|Learning")
    void RecordEscapeRoute(FName RouteId);

    UFUNCTION(BlueprintCallable, Category = "EVA|Learning")
    void RecordHideSpot(FName HideSpotId);

    UFUNCTION(BlueprintCallable, Category = "EVA|Learning")
    void RecordDamageTaken(float DamageAmount, FName DamageSource);

    UFUNCTION(BlueprintCallable, Category = "EVA|Learning")
    void RecordHunterObservation(EEvaCombatStyle ObservedStyle, float DistanceToPlayer,
        FName EscapeRouteId, FName HideSpotId);

    UFUNCTION(BlueprintCallable, Category = "EVA|Learning")
    void RecordHunterTelemetrySnapshot(const FEvaTelemetrySnapshot& ObservedTelemetry);

    UFUNCTION(BlueprintCallable, Category = "EVA|Learning")
    void RecordAnalysisCoreRecovered(int32 SourceHunterTier);

    UFUNCTION(BlueprintCallable, Category = "EVA|Learning")
    void SetLearningSpeedMultiplier(float NewMultiplier);

    UFUNCTION(BlueprintCallable, Category = "EVA|Hunter")
    void SetHunterState(EEvaHunterState NewState, int32 NewHunterTier);

    UFUNCTION(BlueprintCallable, Category = "EVA|Debug")
    void DebugAddAnalysis(float Amount);

    UFUNCTION(BlueprintPure, Category = "EVA|Learning")
    float GetLearningSpeedMultiplier() const { return LearningSpeedMultiplier; }

    UFUNCTION(BlueprintPure, Category = "EVA|Learning")
    float GetEvaAnalysisRate() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Learning")
    float GetObservationMass() const { return ObservationMass; }

    UFUNCTION(BlueprintPure, Category = "EVA|Learning")
    FEvaTelemetrySnapshot GetAggregateTelemetry() const { return AggregateTelemetry; }

    UFUNCTION(BlueprintPure, Category = "EVA|Learning")
    float GetHeadshotRate() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Learning")
    float GetAccuracy() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Learning")
    FName GetDominantWeapon() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Learning")
    EEvaCombatStyle ClassifyAggregateCombatStyle() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Learning")
    EEvaAnalysisStage GetAnalysisStage() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Learning")
    EEvaAdaptationDirective GetAdaptationDirective() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Learning")
    EEvaEvolutionType GetRecommendedEvolutionType() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Hunter")
    EEvaHunterState GetHunterState() const { return HunterState; }

    UFUNCTION(BlueprintPure, Category = "EVA|Hunter")
    int32 GetHunterTier() const { return HunterTier; }

    UFUNCTION(BlueprintPure, Category = "EVA|Hunter")
    bool IsHunterActive() const { return HunterState == EEvaHunterState::Deployed; }

private:
    void AddObservationMass(float BaseAmount, float ObserverAccuracy = 0.35f);

    UPROPERTY(VisibleAnywhere, Category = "EVA|Learning")
    FEvaTelemetrySnapshot AggregateTelemetry;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Learning")
    float LearningSpeedMultiplier = 1.0f;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Learning")
    float ObservationMass = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Hunter")
    EEvaHunterState HunterState = EEvaHunterState::Dormant;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Hunter")
    int32 HunterTier = 0;
};
