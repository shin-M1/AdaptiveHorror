#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/EvaTelemetryTypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "EvaZombieAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
struct FPathFollowingResult;

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaZombieAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEvaZombieAIController();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "EVA|AI")
    void SetPlayerTarget(AActor* NewTarget);

    UFUNCTION(BlueprintCallable, Category = "EVA|AI")
    void ClearPlayerTarget();

    UFUNCTION(BlueprintCallable, Category = "EVA|AI")
    void StopCombatForStageClear();

    UFUNCTION(BlueprintPure, Category = "EVA|AI")
    AActor* GetPlayerTarget() const { return TargetActor; }

    UFUNCTION(BlueprintPure, Category = "EVA|AI")
    bool IsCombatEnabled() const { return bCombatEnabled; }

    UFUNCTION(BlueprintCallable, Category = "EVA|AI")
    void ConfigureCombat(float NewAttackRange, float NewAttackDamage, float NewAttackInterval);

    UFUNCTION(BlueprintCallable, Category = "EVA|AI")
    void ConfigurePerception(float NewSightRadius, float NewHearingRange);

    UFUNCTION(BlueprintCallable, Category = "EVA|Adaptation")
    bool ApplyCurrentGameplayAdaptation(bool bForceApply = false);

    UFUNCTION(BlueprintPure, Category = "EVA|Adaptation")
    FEvaPlayerAdaptationProfile GetCurrentAdaptationProfile() const { return LastAdaptationProfile; }

    UFUNCTION(BlueprintPure, Category = "EVA|Adaptation")
    FEvaEnemyAdaptationTuning GetCurrentAdaptationTuning() const { return CurrentAdaptationTuning; }

    UFUNCTION(BlueprintPure, Category = "EVA|Adaptation")
    FString GetCurrentAdaptationSummary() const { return CurrentAdaptationTuning.DebugSummary; }

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

    UFUNCTION()
    void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    virtual bool CanAttackTarget() const;
    virtual void TryAttackTarget();
    virtual bool TryHandleLearningAdaptation();
    virtual void ApplyAdaptivePerception();
    bool MoveToActorOrDirect(AActor* GoalActor, float AcceptanceRadius);
    bool MoveToLocationOrDirect(const FVector& GoalLocation, float AcceptanceRadius);
    bool TrySidestepAroundObstacle(const FVector& GoalLocation);
    bool ApplyDirectFallbackMovement(const FVector& DesiredDirection, const FColor& DebugColor);
    bool CanUseDirectFallback(const FVector& DesiredDirection, float TraceDistance, FString& OutReason) const;
    bool ProjectNavigationPoint(const FVector& Point, FVector& OutProjectedLocation) const;
    bool EvaluateRepathForStationaryTarget(float DeltaSeconds);
    bool ReissueMoveToTarget(const TCHAR* RepathReason, bool bAbortCurrentMove);
    bool TryMoveForAdaptationRole(const FEvaEnemyAdaptationTuning& Tuning, const FVector& PawnLocation,
        const FVector& TargetLocation);
    void LogPathDiagnostics(const TCHAR* Context, const FVector& GoalLocation, EPathFollowingRequestResult::Type MoveResult) const;
    void LogRepathState(const TCHAR* RepathReason, EPathFollowingRequestResult::Type MoveResult, float RecentMoveDistance) const;
    AActor* FindNearestTaggedActor(FName Tag, const FVector& FromLocation) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|AI")
    TObjectPtr<UAIPerceptionComponent> EvaPerceptionComponent;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|AI")
    TObjectPtr<AActor> TargetActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|AI")
    float AttackRange = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|AI")
    float AttackDamage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|AI")
    float AttackInterval = 1.5f;

private:
    float BaseConfiguredAttackRange = 150.0f;
    float BaseConfiguredAttackDamage = 10.0f;
    float BaseConfiguredAttackInterval = 1.5f;
    float BaseConfiguredMoveSpeed = 0.0f;
    float LastAttackTime = -1000.0f;
    float LastAdaptiveMoveTime = -1000.0f;
    float LastAdaptationApplyTime = -1000.0f;
    FVector LastObservedPawnLocation = FVector::ZeroVector;
    float TimeSinceMeaningfulMovement = 0.0f;
    EEvaAdaptationDirective LastAppliedDirective = EEvaAdaptationDirective::None;
    FEvaPlayerAdaptationProfile LastAdaptationProfile;
    FEvaEnemyAdaptationTuning CurrentAdaptationTuning;
    float LastMoveRequestTime = -1000.0f;
    float LastMoveDiagnosticLogTime = -1000.0f;
    FVector LastMoveRequestGoal = FVector::ZeroVector;
    int32 ConsecutiveMoveFailures = 0;
    bool bPreferRightDetour = true;
    bool bRecoveringSidestep = false;
    float LastSidestepMoveTime = -1000.0f;
    bool bDirectFallbackActive = false;
    float LastDirectFallbackTime = -1000.0f;
    float LastRepathMonitorTime = -1000.0f;
    float LastMeaningfulProgressTime = -1000.0f;
    float LastProgressSampleTime = -1000.0f;
    float LastProgressDistance = 0.0f;
    FVector LastProgressSampleLocation = FVector::ZeroVector;
    FVector LastRepathTargetLocation = FVector::ZeroVector;
    bool bInternalRepathAbort = false;
    bool bIssuingRepathMove = false;
    bool bCombatEnabled = true;
};
