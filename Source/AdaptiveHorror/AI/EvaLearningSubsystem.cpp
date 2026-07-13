#include "AI/EvaLearningSubsystem.h"
#include "Engine/Engine.h"

void UEvaLearningSubsystem::RecordShot(const FName WeaponName)
{
    ++AggregateTelemetry.ShotCount;
    AggregateTelemetry.WeaponUseCount.FindOrAdd(WeaponName)++;
    AddObservationMass(1.0f);
}

void UEvaLearningSubsystem::RecordHit(const bool bHeadshot, const float CombatDistance)
{
    ++AggregateTelemetry.HitCount;
    AggregateTelemetry.HeadshotCount += bHeadshot ? 1 : 0;
    AggregateTelemetry.CombatDistanceTotal += FMath::Max(0.0f, CombatDistance);
    ++AggregateTelemetry.CombatDistanceSamples;
    AggregateTelemetry.AverageCombatDistance = AggregateTelemetry.CombatDistanceTotal /
        FMath::Max(1, AggregateTelemetry.CombatDistanceSamples);
    AddObservationMass(bHeadshot ? 3.0f : 2.0f);
}

void UEvaLearningSubsystem::RecordKill()
{
    ++AggregateTelemetry.KillCount;
    AddObservationMass(8.0f);
}

void UEvaLearningSubsystem::RecordDeathCause(const FName Cause)
{
    AggregateTelemetry.DeathCause = Cause;
    AddObservationMass(5.0f);
}

void UEvaLearningSubsystem::RecordEscapeRoute(const FName RouteId)
{
    AggregateTelemetry.LastKnownEscapeRoute = RouteId;
    AddObservationMass(2.0f);
}

void UEvaLearningSubsystem::RecordHideSpot(const FName HideSpotId)
{
    AggregateTelemetry.LastUsedHideSpot = HideSpotId;
    AddObservationMass(2.0f);
}

void UEvaLearningSubsystem::RecordDamageTaken(const float DamageAmount, const FName DamageSource)
{
    if (DamageAmount <= 0.0f)
    {
        return;
    }

    AggregateTelemetry.PlayerDamageTakenTotal += DamageAmount;
    ++AggregateTelemetry.DamageTakenSamples;
    AggregateTelemetry.LastDamageSource = DamageSource;
    AddObservationMass(1.5f);
}

void UEvaLearningSubsystem::RecordHunterObservation(const EEvaCombatStyle ObservedStyle,
    const float DistanceToPlayer, const FName EscapeRouteId, const FName HideSpotId)
{
    if (DistanceToPlayer > 0.0f)
    {
        AggregateTelemetry.CombatDistanceTotal += DistanceToPlayer;
        ++AggregateTelemetry.CombatDistanceSamples;
        AggregateTelemetry.AverageCombatDistance = AggregateTelemetry.CombatDistanceTotal /
            FMath::Max(1, AggregateTelemetry.CombatDistanceSamples);
    }

    if (!EscapeRouteId.IsNone())
    {
        AggregateTelemetry.LastKnownEscapeRoute = EscapeRouteId;
    }
    if (!HideSpotId.IsNone())
    {
        AggregateTelemetry.LastUsedHideSpot = HideSpotId;
    }

    float ObservationValue = 5.0f;
    if (ObservedStyle == EEvaCombatStyle::Ghost || ObservedStyle == EEvaCombatStyle::Explorer)
    {
        ObservationValue = 6.0f;
    }
    AddObservationMass(ObservationValue, 1.0f);
}

void UEvaLearningSubsystem::RecordHunterTelemetrySnapshot(const FEvaTelemetrySnapshot& ObservedTelemetry)
{
    for (const TPair<FName, int32>& Entry : ObservedTelemetry.WeaponUseCount)
    {
        int32& ExistingCount = AggregateTelemetry.WeaponUseCount.FindOrAdd(Entry.Key);
        ExistingCount = FMath::Max(ExistingCount, Entry.Value);
    }

    AggregateTelemetry.ShotCount = FMath::Max(AggregateTelemetry.ShotCount, ObservedTelemetry.ShotCount);
    AggregateTelemetry.HitCount = FMath::Max(AggregateTelemetry.HitCount, ObservedTelemetry.HitCount);
    AggregateTelemetry.HeadshotCount = FMath::Max(AggregateTelemetry.HeadshotCount, ObservedTelemetry.HeadshotCount);
    AggregateTelemetry.KillCount = FMath::Max(AggregateTelemetry.KillCount, ObservedTelemetry.KillCount);
    AggregateTelemetry.PlayerDamageTakenTotal =
        FMath::Max(AggregateTelemetry.PlayerDamageTakenTotal, ObservedTelemetry.PlayerDamageTakenTotal);
    AggregateTelemetry.DamageTakenSamples =
        FMath::Max(AggregateTelemetry.DamageTakenSamples, ObservedTelemetry.DamageTakenSamples);

    if (ObservedTelemetry.CombatDistanceSamples > AggregateTelemetry.CombatDistanceSamples)
    {
        AggregateTelemetry.CombatDistanceTotal = ObservedTelemetry.CombatDistanceTotal;
        AggregateTelemetry.CombatDistanceSamples = ObservedTelemetry.CombatDistanceSamples;
        AggregateTelemetry.AverageCombatDistance = ObservedTelemetry.AverageCombatDistance;
    }
    if (!ObservedTelemetry.LastDamageSource.IsNone())
    {
        AggregateTelemetry.LastDamageSource = ObservedTelemetry.LastDamageSource;
    }
    if (!ObservedTelemetry.LastKnownEscapeRoute.IsNone())
    {
        AggregateTelemetry.LastKnownEscapeRoute = ObservedTelemetry.LastKnownEscapeRoute;
    }
    if (!ObservedTelemetry.LastUsedHideSpot.IsNone())
    {
        AggregateTelemetry.LastUsedHideSpot = ObservedTelemetry.LastUsedHideSpot;
    }

    AddObservationMass(2.5f, 1.0f);
}

void UEvaLearningSubsystem::RecordAnalysisCoreRecovered(const int32 SourceHunterTier)
{
    // The core is a player reward and a design hook for future crafting.
    // It still exposes post-combat data, so EVA receives a small delayed sample.
    AddObservationMass(2.0f + FMath::Max(0, SourceHunterTier - 1), 0.15f);
}

void UEvaLearningSubsystem::SetLearningSpeedMultiplier(const float NewMultiplier)
{
    LearningSpeedMultiplier = FMath::Clamp(NewMultiplier, 0.0f, 2.0f);
}

void UEvaLearningSubsystem::SetHunterState(const EEvaHunterState NewState, const int32 NewHunterTier)
{
    HunterState = NewState;
    HunterTier = FMath::Max(0, NewHunterTier);
}

void UEvaLearningSubsystem::DebugAddAnalysis(const float Amount)
{
#if UE_BUILD_SHIPPING
    (void)Amount;
#else
    ObservationMass = FMath::Clamp(ObservationMass + Amount, 0.0f, 100.0f);
#endif
}

void UEvaLearningSubsystem::ResetLearning()
{
    AggregateTelemetry = FEvaTelemetrySnapshot();
    LearningSpeedMultiplier = 1.0f;
    ObservationMass = 0.0f;
    HunterState = EEvaHunterState::Dormant;
    HunterTier = 0;
}

float UEvaLearningSubsystem::GetEvaAnalysisRate() const
{
    return FMath::Clamp(ObservationMass, 0.0f, 100.0f);
}

float UEvaLearningSubsystem::GetHeadshotRate() const
{
    return AggregateTelemetry.HitCount > 0 ?
        static_cast<float>(AggregateTelemetry.HeadshotCount) / AggregateTelemetry.HitCount : 0.0f;
}

float UEvaLearningSubsystem::GetAccuracy() const
{
    return AggregateTelemetry.ShotCount > 0 ?
        static_cast<float>(AggregateTelemetry.HitCount) / AggregateTelemetry.ShotCount : 0.0f;
}

FName UEvaLearningSubsystem::GetDominantWeapon() const
{
    FName Dominant = NAME_None;
    int32 HighestCount = 0;
    for (const TPair<FName, int32>& Entry : AggregateTelemetry.WeaponUseCount)
    {
        if (Entry.Value > HighestCount)
        {
            Dominant = Entry.Key;
            HighestCount = Entry.Value;
        }
    }
    return Dominant;
}

EEvaCombatStyle UEvaLearningSubsystem::ClassifyAggregateCombatStyle() const
{
    if (!AggregateTelemetry.LastUsedHideSpot.IsNone())
    {
        return EEvaCombatStyle::Ghost;
    }
    if (!AggregateTelemetry.LastKnownEscapeRoute.IsNone())
    {
        return EEvaCombatStyle::Explorer;
    }
    if (AggregateTelemetry.CombatDistanceSamples <= 0)
    {
        return EEvaCombatStyle::Unknown;
    }
    if (GetDominantWeapon() == FName(TEXT("Shotgun")) || AggregateTelemetry.AverageCombatDistance < 500.0f)
    {
        return EEvaCombatStyle::Berserker;
    }
    if (AggregateTelemetry.AverageCombatDistance > 1500.0f)
    {
        return EEvaCombatStyle::Ranger;
    }
    return EEvaCombatStyle::Tactician;
}

EEvaAnalysisStage UEvaLearningSubsystem::GetAnalysisStage() const
{
    const float Rate = GetEvaAnalysisRate();
    if (Rate >= 60.0f)
    {
        return EEvaAnalysisStage::Evolving;
    }
    if (Rate >= 20.0f)
    {
        return EEvaAnalysisStage::Adapting;
    }
    return EEvaAnalysisStage::Learning;
}

EEvaAdaptationDirective UEvaLearningSubsystem::GetAdaptationDirective() const
{
    if (GetEvaAnalysisRate() < 20.0f)
    {
        return EEvaAdaptationDirective::None;
    }

    switch (ClassifyAggregateCombatStyle())
    {
    case EEvaCombatStyle::Berserker:
        return EEvaAdaptationDirective::CounterCloseRange;
    case EEvaCombatStyle::Ranger:
        return EEvaAdaptationDirective::CounterLongRange;
    case EEvaCombatStyle::Ghost:
        return EEvaAdaptationDirective::CounterStealth;
    case EEvaCombatStyle::Explorer:
        return EEvaAdaptationDirective::CounterExplorer;
    default:
        return EEvaAdaptationDirective::None;
    }
}

EEvaEvolutionType UEvaLearningSubsystem::GetRecommendedEvolutionType() const
{
    const float Rate = GetEvaAnalysisRate();
    if (Rate >= 80.0f)
    {
        return EEvaEvolutionType::Composite;
    }
    if (Rate >= 60.0f)
    {
        return EEvaEvolutionType::LongArm;
    }
    if (Rate >= 40.0f)
    {
        return EEvaEvolutionType::Armored;
    }
    if (Rate >= 20.0f)
    {
        return EEvaEvolutionType::Fast;
    }
    return EEvaEvolutionType::None;
}

void UEvaLearningSubsystem::AddObservationMass(const float BaseAmount, const float ObserverAccuracy)
{
    const float PreviousMass = ObservationMass;
    ObservationMass = FMath::Clamp(ObservationMass +
        BaseAmount * FMath::Clamp(ObserverAccuracy, 0.0f, 1.0f) * LearningSpeedMultiplier, 0.0f, 100.0f);
#if !UE_BUILD_SHIPPING
    const int32 PreviousBucket = FMath::FloorToInt(PreviousMass / 10.0f);
    const int32 NewBucket = FMath::FloorToInt(ObservationMass / 10.0f);
    if (GEngine && NewBucket > PreviousBucket)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan,
            FString::Printf(TEXT("EVA analysis increased: %.0f%%"), ObservationMass));
    }
#endif
}
