#include "AI/EvaLearningSubsystem.h"
#include "AdaptiveHorror.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
FString EvaShortCombatStyle(const EEvaCombatStyle Style)
{
    switch (Style)
    {
    case EEvaCombatStyle::Berserker:
        return TEXT("Berserker");
    case EEvaCombatStyle::Tactician:
        return TEXT("Tactician");
    case EEvaCombatStyle::Ranger:
        return TEXT("Ranger");
    case EEvaCombatStyle::Ghost:
        return TEXT("Ghost");
    case EEvaCombatStyle::Explorer:
        return TEXT("Explorer");
    default:
        return TEXT("Unknown");
    }
}

FString EvaShortRoleLabel(const EEvaEnemyBehaviorRole Role, const FString& CompositeHybridType)
{
    switch (Role)
    {
    case EEvaEnemyBehaviorRole::Flanker:
        return TEXT("FLANK");
    case EEvaEnemyBehaviorRole::Frontliner:
        return TEXT("HOLD FRONT");
    case EEvaEnemyBehaviorRole::MidRangePressure:
        return TEXT("KEEP DISTANCE");
    case EEvaEnemyBehaviorRole::Searcher:
        return TEXT("SEARCH LAST SEEN");
    case EEvaEnemyBehaviorRole::Ambusher:
        return TEXT("AMBUSH");
    case EEvaEnemyBehaviorRole::CompositeAdaptive:
        return CompositeHybridType.IsEmpty() ? FString(TEXT("COMPOSITE HYBRID")) : CompositeHybridType;
    default:
        return TEXT("CHASE");
    }
}
}

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

void UEvaLearningSubsystem::RecordSprintUsed()
{
    ++AggregateTelemetry.SprintUseCount;
    AddObservationMass(0.35f);
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
    AggregateTelemetry.SprintUseCount =
        FMath::Max(AggregateTelemetry.SprintUseCount, ObservedTelemetry.SprintUseCount);

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
    CachedAdaptationProfile = FEvaPlayerAdaptationProfile();
    LastHunterDefeatedProfile = FEvaPlayerAdaptationProfile();
    LastProfileUpdateTime = -1000.0f;
    bProfileUpdatesEnabled = true;
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[EVAProfile] Reset"));
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
    return ClassifyAdaptationProfile(BuildProfileFromTelemetry());
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

FEvaPlayerAdaptationProfile UEvaLearningSubsystem::UpdateAdaptationProfile(const bool bForceUpdate)
{
    const UWorld* World = GetWorld();
    const float Now = World ? World->GetTimeSeconds() : 0.0f;
    if (!bForceUpdate && !bProfileUpdatesEnabled)
    {
        return CachedAdaptationProfile;
    }
    if (!bForceUpdate && CachedAdaptationProfile.bValid && Now - LastProfileUpdateTime < ProfileUpdateInterval)
    {
        return CachedAdaptationProfile;
    }

    const FEvaPlayerAdaptationProfile PreviousProfile = CachedAdaptationProfile;
    CachedAdaptationProfile = BuildProfileFromTelemetry();
    LastProfileUpdateTime = Now;

    const bool bStyleChanged = PreviousProfile.CombatStyle != CachedAdaptationProfile.CombatStyle;
    const bool bStageChanged = PreviousProfile.EvaStage != CachedAdaptationProfile.EvaStage;
    const int32 PreviousAnalysisBucket = FMath::FloorToInt(PreviousProfile.AnalysisPercent / 10.0f);
    const int32 NewAnalysisBucket = FMath::FloorToInt(CachedAdaptationProfile.AnalysisPercent / 10.0f);
    if (bForceUpdate || bStyleChanged || bStageChanged || PreviousAnalysisBucket != NewAnalysisBucket)
    {
        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[EVAProfile] Updated Valid=%s Style=%s Stage=%s Analysis=%.1f Accuracy=%.2f HS=%.2f Distance=%.1f Close=%.2f Long=%.2f DamageTaken=%.2f Sprint=%.2f Aggression=%.2f Stealth=%.2f Exploration=%.2f Weapon=%s"),
            CachedAdaptationProfile.bValid ? TEXT("true") : TEXT("false"),
            *UEnum::GetValueAsString(CachedAdaptationProfile.CombatStyle),
            *UEnum::GetValueAsString(CachedAdaptationProfile.EvaStage),
            CachedAdaptationProfile.AnalysisPercent,
            CachedAdaptationProfile.Accuracy,
            CachedAdaptationProfile.HeadshotRate,
            CachedAdaptationProfile.PreferredCombatDistance,
            CachedAdaptationProfile.CloseRangeRatio,
            CachedAdaptationProfile.LongRangeRatio,
            CachedAdaptationProfile.DamageTakenRate,
            CachedAdaptationProfile.SprintUsage,
            CachedAdaptationProfile.AggressionScore,
            CachedAdaptationProfile.StealthScore,
            CachedAdaptationProfile.ExplorationScore,
            *CachedAdaptationProfile.MostUsedWeapon.ToString());
    }

    return CachedAdaptationProfile;
}

void UEvaLearningSubsystem::SetProfileUpdatesEnabled(const bool bEnabled)
{
    bProfileUpdatesEnabled = bEnabled;
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[EVAProfile] UpdatesEnabled=%s"), bEnabled ? TEXT("true") : TEXT("false"));
}

void UEvaLearningSubsystem::RecordHunterDefeatedProfile()
{
    LastHunterDefeatedProfile = CachedAdaptationProfile.bValid ? CachedAdaptationProfile : BuildProfileFromTelemetry();
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[HunterAdapt] DefeatProfile Stored=%s Style=%s Analysis=%.1f"),
        LastHunterDefeatedProfile.bValid ? TEXT("true") : TEXT("false"),
        *UEnum::GetValueAsString(LastHunterDefeatedProfile.CombatStyle),
        LastHunterDefeatedProfile.AnalysisPercent);
}

FEvaEnemyAdaptationTuning UEvaLearningSubsystem::BuildEnemyAdaptationTuning(
    const EEvaEvolutionType EvolutionType) const
{
    const FEvaPlayerAdaptationProfile Profile = CachedAdaptationProfile.bValid ?
        CachedAdaptationProfile : BuildProfileFromTelemetry();

    FEvaEnemyAdaptationTuning Tuning;
    Tuning.EvolutionType = EvolutionType;
    Tuning.CounteredStyle = Profile.CombatStyle;
    Tuning.HunterCounterType = CounterTypeFromCombatStyle(Profile.CombatStyle);

    switch (Profile.CombatStyle)
    {
    case EEvaCombatStyle::Berserker:
        Tuning.BehaviorRole = EEvaEnemyBehaviorRole::Frontliner;
        Tuning.AttackCooldownMultiplier = 0.92f;
        Tuning.SidestepChance = 0.22f;
        Tuning.SearchDuration = 3.5f;
        break;
    case EEvaCombatStyle::Ranger:
        Tuning.BehaviorRole = EEvaEnemyBehaviorRole::Flanker;
        Tuning.MoveSpeedMultiplier = 1.07f;
        Tuning.AttackCooldownMultiplier = 0.96f;
        Tuning.SidestepChance = 0.45f;
        Tuning.SearchDuration = 3.0f;
        break;
    case EEvaCombatStyle::Ghost:
        Tuning.BehaviorRole = EEvaEnemyBehaviorRole::Searcher;
        Tuning.MoveSpeedMultiplier = 1.04f;
        Tuning.SidestepChance = 0.28f;
        Tuning.SearchDuration = 7.5f;
        break;
    case EEvaCombatStyle::Explorer:
        Tuning.BehaviorRole = EEvaEnemyBehaviorRole::Ambusher;
        Tuning.MoveSpeedMultiplier = 1.02f;
        Tuning.SidestepChance = 0.25f;
        Tuning.SearchDuration = 5.5f;
        break;
    default:
        break;
    }

    switch (EvolutionType)
    {
    case EEvaEvolutionType::Fast:
        Tuning.BehaviorRole = EEvaEnemyBehaviorRole::Flanker;
        Tuning.MoveSpeedMultiplier *= 1.22f;
        Tuning.AttackCooldownMultiplier *= 0.92f;
        Tuning.SidestepChance = FMath::Max(Tuning.SidestepChance,
            Profile.CombatStyle == EEvaCombatStyle::Ranger ? 0.58f : 0.50f);
        break;
    case EEvaEvolutionType::Armored:
        Tuning.BehaviorRole = EEvaEnemyBehaviorRole::Frontliner;
        Tuning.MoveSpeedMultiplier *= 0.86f;
        Tuning.HealthMultiplier *= 1.22f;
        Tuning.AttackCooldownMultiplier *= 1.06f;
        Tuning.SidestepChance *= 0.35f;
        break;
    case EEvaEvolutionType::LongArm:
        Tuning.BehaviorRole = EEvaEnemyBehaviorRole::MidRangePressure;
        Tuning.AttackRangeMultiplier *= 1.36f;
        Tuning.DamageMultiplier *= 1.08f;
        Tuning.AttackCooldownMultiplier *= 1.06f;
        break;
    case EEvaEvolutionType::Composite:
        Tuning.BehaviorRole = EEvaEnemyBehaviorRole::CompositeAdaptive;
        Tuning.CompositeHybridHoldSeconds = 7.0f;
        if (Profile.CombatStyle == EEvaCombatStyle::Ranger)
        {
            Tuning.CompositeHybridType = TEXT("ANTI-RANGER");
            Tuning.CompositeHybridRoleCount = 2;
            Tuning.MoveSpeedMultiplier *= 1.16f;
            Tuning.HealthMultiplier *= 1.06f;
            Tuning.SidestepChance = FMath::Max(Tuning.SidestepChance, 0.56f);
        }
        else if (Profile.CombatStyle == EEvaCombatStyle::Berserker)
        {
            Tuning.CompositeHybridType = TEXT("ANTI-BERSERKER");
            Tuning.CompositeHybridRoleCount = 2;
            Tuning.AttackRangeMultiplier *= 1.18f;
            Tuning.SidestepChance = FMath::Max(Tuning.SidestepChance, 0.48f);
            Tuning.AttackCooldownMultiplier *= 1.02f;
        }
        else if (Profile.CombatStyle == EEvaCombatStyle::Ghost)
        {
            Tuning.CompositeHybridType = TEXT("ANTI-GHOST");
            Tuning.CompositeHybridRoleCount = 2;
            Tuning.SearchDuration = FMath::Max(Tuning.SearchDuration, 8.5f);
            Tuning.MoveSpeedMultiplier *= 1.10f;
            Tuning.SidestepChance = FMath::Max(Tuning.SidestepChance, 0.40f);
        }
        else if (Profile.CombatStyle == EEvaCombatStyle::Explorer)
        {
            Tuning.CompositeHybridType = TEXT("ANTI-EXPLORER");
            Tuning.CompositeHybridRoleCount = 2;
            Tuning.HealthMultiplier *= 1.08f;
            Tuning.AttackRangeMultiplier *= 1.08f;
            Tuning.SearchDuration = FMath::Max(Tuning.SearchDuration, 6.5f);
            Tuning.SidestepChance = FMath::Max(Tuning.SidestepChance, 0.28f);
        }
        else
        {
            Tuning.CompositeHybridType = TEXT("TACTICAL HYBRID");
            Tuning.CompositeHybridRoleCount = 1;
            Tuning.MoveSpeedMultiplier *= 1.07f;
            Tuning.AttackRangeMultiplier *= 1.08f;
        }
        break;
    default:
        break;
    }

    const float AnalysisAlpha = FMath::Clamp(Profile.AnalysisPercent / 100.0f, 0.0f, 1.0f);
    Tuning.MoveSpeedMultiplier = FMath::Lerp(1.0f, Tuning.MoveSpeedMultiplier, 0.45f + AnalysisAlpha * 0.45f);
    Tuning.AttackRangeMultiplier = FMath::Lerp(1.0f, Tuning.AttackRangeMultiplier, 0.45f + AnalysisAlpha * 0.45f);
    Tuning.AttackCooldownMultiplier = FMath::Lerp(1.0f, Tuning.AttackCooldownMultiplier, 0.45f + AnalysisAlpha * 0.35f);
    Tuning.DamageMultiplier = FMath::Lerp(1.0f, Tuning.DamageMultiplier, 0.35f + AnalysisAlpha * 0.30f);
    Tuning.SidestepChance *= 0.65f + AnalysisAlpha * 0.35f;

    ClampTuning(Tuning);
    Tuning.RoleLabel = EvaShortRoleLabel(Tuning.BehaviorRole, Tuning.CompositeHybridType);
    Tuning.IntentLabel = Tuning.RoleLabel;
    Tuning.DebugSummary = FString::Printf(TEXT("%s vs %s spd%.2f rng%.2f cd%.2f side%.2f"),
        *Tuning.RoleLabel,
        *EvaShortCombatStyle(Tuning.CounteredStyle),
        Tuning.MoveSpeedMultiplier,
        Tuning.AttackRangeMultiplier,
        Tuning.AttackCooldownMultiplier,
        Tuning.SidestepChance);
    return Tuning;
}

EEvaHunterCounterType UEvaLearningSubsystem::GetHunterCounterTypeForTier(const int32 RequestedHunterTier) const
{
    const FEvaPlayerAdaptationProfile SourceProfile =
        RequestedHunterTier >= 2 && LastHunterDefeatedProfile.bValid ?
        LastHunterDefeatedProfile :
        (CachedAdaptationProfile.bValid ? CachedAdaptationProfile : BuildProfileFromTelemetry());
    return CounterTypeFromCombatStyle(SourceProfile.CombatStyle);
}

FString UEvaLearningSubsystem::GetProfileDebugString() const
{
    const FEvaPlayerAdaptationProfile Profile = CachedAdaptationProfile.bValid ?
        CachedAdaptationProfile : BuildProfileFromTelemetry();
    return FString::Printf(TEXT("Style=%s Stage=%s Analysis=%.0f Acc=%.1f HS=%.1f Dist=%.0f Agg=%.2f Stealth=%.2f Explore=%.2f"),
        *UEnum::GetValueAsString(Profile.CombatStyle),
        *UEnum::GetValueAsString(Profile.EvaStage),
        Profile.AnalysisPercent,
        Profile.Accuracy * 100.0f,
        Profile.HeadshotRate * 100.0f,
        Profile.PreferredCombatDistance,
        Profile.AggressionScore,
        Profile.StealthScore,
        Profile.ExplorationScore);
}

FEvaPlayerAdaptationProfile UEvaLearningSubsystem::BuildProfileFromTelemetry() const
{
    FEvaPlayerAdaptationProfile Profile;
    Profile.HeadshotRate = FMath::Clamp(GetHeadshotRate(), 0.0f, 1.0f);
    Profile.Accuracy = FMath::Clamp(GetAccuracy(), 0.0f, 1.0f);
    Profile.PreferredCombatDistance = FMath::Clamp(AggregateTelemetry.AverageCombatDistance, 0.0f, 5000.0f);
    Profile.MostUsedWeapon = GetDominantWeapon();
    Profile.AnalysisPercent = GetEvaAnalysisRate();
    Profile.EvaStage = GetAnalysisStage();

    if (AggregateTelemetry.CombatDistanceSamples > 0)
    {
        Profile.CloseRangeRatio = FMath::Clamp(
            1.0f - (Profile.PreferredCombatDistance - 350.0f) / 900.0f, 0.0f, 1.0f);
        Profile.LongRangeRatio = FMath::Clamp(
            (Profile.PreferredCombatDistance - 1100.0f) / 1200.0f, 0.0f, 1.0f);
    }

    const float DamageBaseline = FMath::Max(1.0f, static_cast<float>(AggregateTelemetry.DamageTakenSamples) * 100.0f);
    Profile.DamageTakenRate = FMath::Clamp(AggregateTelemetry.PlayerDamageTakenTotal / DamageBaseline, 0.0f, 1.0f);

    const float ActivityCount = FMath::Max(1.0f,
        static_cast<float>(AggregateTelemetry.ShotCount + AggregateTelemetry.KillCount + AggregateTelemetry.SprintUseCount));
    Profile.SprintUsage = FMath::Clamp(static_cast<float>(AggregateTelemetry.SprintUseCount) / ActivityCount, 0.0f, 1.0f);

    const float LowShotScore = AggregateTelemetry.ShotCount <= 3 && AggregateTelemetry.KillCount == 0 ? 0.25f : 0.0f;
    Profile.AggressionScore = FMath::Clamp(Profile.CloseRangeRatio * 0.45f +
        Profile.DamageTakenRate * 0.25f + Profile.SprintUsage * 0.20f +
        (AggregateTelemetry.ShotCount >= 12 ? 0.10f : 0.0f), 0.0f, 1.0f);
    Profile.StealthScore = FMath::Clamp((!AggregateTelemetry.LastUsedHideSpot.IsNone() ? 0.48f : 0.0f) +
        LowShotScore + (Profile.DamageTakenRate < 0.15f ? 0.12f : 0.0f), 0.0f, 1.0f);
    Profile.ExplorationScore = FMath::Clamp((!AggregateTelemetry.LastKnownEscapeRoute.IsNone() ? 0.48f : 0.0f) +
        (Profile.CloseRangeRatio > 0.25f && Profile.LongRangeRatio > 0.25f ? 0.18f : 0.0f) +
        (AggregateTelemetry.CombatDistanceSamples >= 6 ? 0.12f : 0.0f), 0.0f, 1.0f);
    Profile.bValid = AggregateTelemetry.ShotCount > 0 || AggregateTelemetry.HitCount > 0 ||
        AggregateTelemetry.KillCount > 0 || AggregateTelemetry.DamageTakenSamples > 0 ||
        AggregateTelemetry.SprintUseCount > 0 || Profile.AnalysisPercent > 0.0f ||
        !AggregateTelemetry.LastKnownEscapeRoute.IsNone() || !AggregateTelemetry.LastUsedHideSpot.IsNone();
    Profile.CombatStyle = ClassifyAdaptationProfile(Profile);
    return Profile;
}

EEvaCombatStyle UEvaLearningSubsystem::ClassifyAdaptationProfile(const FEvaPlayerAdaptationProfile& Profile) const
{
    if (Profile.StealthScore >= 0.55f)
    {
        return EEvaCombatStyle::Ghost;
    }
    if (Profile.ExplorationScore >= 0.55f)
    {
        return EEvaCombatStyle::Explorer;
    }
    if (Profile.LongRangeRatio >= 0.45f && (Profile.Accuracy >= 0.35f || Profile.HeadshotRate >= 0.30f))
    {
        return EEvaCombatStyle::Ranger;
    }
    if (Profile.MostUsedWeapon == FName(TEXT("Shotgun")) || Profile.AggressionScore >= 0.45f ||
        Profile.CloseRangeRatio >= 0.65f)
    {
        return EEvaCombatStyle::Berserker;
    }
    if (AggregateTelemetry.CombatDistanceSamples <= 0 && !Profile.bValid)
    {
        return EEvaCombatStyle::Unknown;
    }
    return EEvaCombatStyle::Tactician;
}

EEvaHunterCounterType UEvaLearningSubsystem::CounterTypeFromCombatStyle(const EEvaCombatStyle Style)
{
    switch (Style)
    {
    case EEvaCombatStyle::Berserker:
        return EEvaHunterCounterType::AntiBerserker;
    case EEvaCombatStyle::Ranger:
        return EEvaHunterCounterType::AntiRanger;
    case EEvaCombatStyle::Ghost:
        return EEvaHunterCounterType::AntiGhost;
    case EEvaCombatStyle::Explorer:
        return EEvaHunterCounterType::AntiExplorer;
    default:
        return EEvaHunterCounterType::None;
    }
}

void UEvaLearningSubsystem::ClampTuning(FEvaEnemyAdaptationTuning& Tuning)
{
    Tuning.MoveSpeedMultiplier = FMath::Clamp(Tuning.MoveSpeedMultiplier, 0.72f, 1.35f);
    Tuning.AttackCooldownMultiplier = FMath::Clamp(Tuning.AttackCooldownMultiplier, 0.75f, 1.35f);
    Tuning.SidestepChance = FMath::Clamp(Tuning.SidestepChance, 0.0f, 0.65f);
    Tuning.SearchDuration = FMath::Clamp(Tuning.SearchDuration, 2.0f, 10.0f);
    Tuning.AttackRangeMultiplier = FMath::Clamp(Tuning.AttackRangeMultiplier, 0.90f, 1.55f);
    Tuning.HealthMultiplier = FMath::Clamp(Tuning.HealthMultiplier, 0.90f, 1.35f);
    Tuning.DamageMultiplier = FMath::Clamp(Tuning.DamageMultiplier, 0.85f, 1.18f);
    Tuning.CompositeHybridRoleCount = FMath::Clamp(Tuning.CompositeHybridRoleCount, 0, 2);
    Tuning.CompositeHybridHoldSeconds = FMath::Clamp(Tuning.CompositeHybridHoldSeconds, 0.0f, 12.0f);
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
