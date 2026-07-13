#pragma once

#include "CoreMinimal.h"
#include "EvaTelemetryTypes.generated.h"

UENUM(BlueprintType)
enum class EEvaCombatStyle : uint8
{
    Unknown UMETA(DisplayName = "Unknown"),
    Berserker UMETA(DisplayName = "Berserker"),
    Tactician UMETA(DisplayName = "Tactician"),
    Ranger UMETA(DisplayName = "Ranger"),
    Ghost UMETA(DisplayName = "Ghost"),
    Explorer UMETA(DisplayName = "Explorer")
};

UENUM(BlueprintType)
enum class EEvaEvolutionType : uint8
{
    None UMETA(DisplayName = "None"),
    Fast UMETA(DisplayName = "Fast"),
    Armored UMETA(DisplayName = "Armored"),
    LongArm UMETA(DisplayName = "Long Arm"),
    Composite UMETA(DisplayName = "Composite")
};

UENUM(BlueprintType)
enum class EEvaAnalysisStage : uint8
{
    Learning UMETA(DisplayName = "Learning"),
    Adapting UMETA(DisplayName = "Adapting"),
    Evolving UMETA(DisplayName = "Evolving")
};

UENUM(BlueprintType)
enum class EEvaHunterState : uint8
{
    Dormant UMETA(DisplayName = "Dormant"),
    Deployed UMETA(DisplayName = "Deployed"),
    DefeatedCooldown UMETA(DisplayName = "Defeated Cooldown")
};

UENUM(BlueprintType)
enum class EEvaAdaptationDirective : uint8
{
    None UMETA(DisplayName = "None"),
    CounterCloseRange UMETA(DisplayName = "Counter Close Range"),
    CounterLongRange UMETA(DisplayName = "Counter Long Range"),
    CounterStealth UMETA(DisplayName = "Counter Stealth"),
    CounterExplorer UMETA(DisplayName = "Counter Explorer")
};

UENUM(BlueprintType)
enum class EEvaEnemyBehaviorRole : uint8
{
    Standard UMETA(DisplayName = "Standard"),
    Flanker UMETA(DisplayName = "Flanker"),
    Frontliner UMETA(DisplayName = "Frontliner"),
    MidRangePressure UMETA(DisplayName = "Mid Range Pressure"),
    Searcher UMETA(DisplayName = "Searcher"),
    Ambusher UMETA(DisplayName = "Ambusher"),
    CompositeAdaptive UMETA(DisplayName = "Composite Adaptive")
};

UENUM(BlueprintType)
enum class EEvaHunterCounterType : uint8
{
    None UMETA(DisplayName = "None"),
    AntiBerserker UMETA(DisplayName = "Anti Berserker"),
    AntiRanger UMETA(DisplayName = "Anti Ranger"),
    AntiGhost UMETA(DisplayName = "Anti Ghost"),
    AntiExplorer UMETA(DisplayName = "Anti Explorer")
};

UENUM(BlueprintType)
enum class EEvaFacilityZone : uint8
{
    EntryLobby UMETA(DisplayName = "Entry Lobby"),
    SecurityCorridor UMETA(DisplayName = "Security Corridor"),
    ObservationLab UMETA(DisplayName = "Observation Lab"),
    ContainmentWard UMETA(DisplayName = "Containment Ward"),
    DataCoreRoom UMETA(DisplayName = "Data Core Room"),
    AdamArena UMETA(DisplayName = "Adam Arena"),
    Clear UMETA(DisplayName = "Clear")
};

USTRUCT(BlueprintType)
struct ADAPTIVEHORROR_API FEvaTelemetrySnapshot
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    TMap<FName, int32> WeaponUseCount;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    int32 ShotCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    int32 HitCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    int32 HeadshotCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    float AverageCombatDistance = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    float CombatDistanceTotal = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    int32 CombatDistanceSamples = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    int32 KillCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    FName DeathCause = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    FName LastKnownEscapeRoute = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    FName LastUsedHideSpot = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    float PlayerDamageTakenTotal = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    int32 DamageTakenSamples = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    FName LastDamageSource = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Telemetry")
    int32 SprintUseCount = 0;
};

USTRUCT(BlueprintType)
struct ADAPTIVEHORROR_API FEvaPlayerAdaptationProfile
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float HeadshotRate = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float Accuracy = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float PreferredCombatDistance = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float CloseRangeRatio = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float LongRangeRatio = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float DamageTakenRate = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float SprintUsage = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float AggressionScore = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float StealthScore = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float ExplorationScore = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    FName MostUsedWeapon = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    EEvaCombatStyle CombatStyle = EEvaCombatStyle::Unknown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float AnalysisPercent = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    EEvaAnalysisStage EvaStage = EEvaAnalysisStage::Learning;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    bool bValid = false;
};

USTRUCT(BlueprintType)
struct ADAPTIVEHORROR_API FEvaEnemyAdaptationTuning
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    EEvaEvolutionType EvolutionType = EEvaEvolutionType::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    EEvaEnemyBehaviorRole BehaviorRole = EEvaEnemyBehaviorRole::Standard;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    EEvaHunterCounterType HunterCounterType = EEvaHunterCounterType::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    EEvaCombatStyle CounteredStyle = EEvaCombatStyle::Unknown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float MoveSpeedMultiplier = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float AttackCooldownMultiplier = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float SidestepChance = 0.10f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float SearchDuration = 3.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float AttackRangeMultiplier = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float HealthMultiplier = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    float DamageMultiplier = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Adaptation")
    FString DebugSummary;
};
