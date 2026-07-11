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
};
