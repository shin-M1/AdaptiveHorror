#include "Components/EvaPlayerTelemetryComponent.h"
#include "AI/EvaLearningSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UEvaPlayerTelemetryComponent::UEvaPlayerTelemetryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEvaPlayerTelemetryComponent::RecordShot(const FName WeaponName)
{
    ++Telemetry.ShotCount;
    Telemetry.WeaponUseCount.FindOrAdd(WeaponName)++;
    if (UEvaLearningSubsystem* Learning = GetLearningSubsystem())
    {
        Learning->RecordShot(WeaponName);
    }
}

void UEvaPlayerTelemetryComponent::RecordHit(const bool bHeadshot, const float CombatDistance)
{
    ++Telemetry.HitCount;
    Telemetry.HeadshotCount += bHeadshot ? 1 : 0;
    Telemetry.CombatDistanceTotal += FMath::Max(0.0f, CombatDistance);
    ++Telemetry.CombatDistanceSamples;
    Telemetry.AverageCombatDistance = Telemetry.CombatDistanceTotal / FMath::Max(1, Telemetry.CombatDistanceSamples);
    if (UEvaLearningSubsystem* Learning = GetLearningSubsystem())
    {
        Learning->RecordHit(bHeadshot, CombatDistance);
    }
}

void UEvaPlayerTelemetryComponent::RecordKill()
{
    ++Telemetry.KillCount;
    if (UEvaLearningSubsystem* Learning = GetLearningSubsystem())
    {
        Learning->RecordKill();
    }
}

void UEvaPlayerTelemetryComponent::RecordDeathCause(const FName Cause)
{
    Telemetry.DeathCause = Cause;
    if (UEvaLearningSubsystem* Learning = GetLearningSubsystem())
    {
        Learning->RecordDeathCause(Cause);
    }
}

void UEvaPlayerTelemetryComponent::RecordEscapeRoute(const FName RouteId)
{
    Telemetry.LastKnownEscapeRoute = RouteId;
    if (UEvaLearningSubsystem* Learning = GetLearningSubsystem())
    {
        Learning->RecordEscapeRoute(RouteId);
    }
}

void UEvaPlayerTelemetryComponent::RecordHideSpot(const FName HideSpotId)
{
    Telemetry.LastUsedHideSpot = HideSpotId;
    if (UEvaLearningSubsystem* Learning = GetLearningSubsystem())
    {
        Learning->RecordHideSpot(HideSpotId);
    }
}

void UEvaPlayerTelemetryComponent::RecordDamageTaken(const float DamageAmount, const FName DamageSource)
{
    if (DamageAmount <= 0.0f)
    {
        return;
    }

    Telemetry.PlayerDamageTakenTotal += DamageAmount;
    ++Telemetry.DamageTakenSamples;
    Telemetry.LastDamageSource = DamageSource;
    if (UEvaLearningSubsystem* Learning = GetLearningSubsystem())
    {
        Learning->RecordDamageTaken(DamageAmount, DamageSource);
    }
}

void UEvaPlayerTelemetryComponent::ResetTelemetry()
{
    Telemetry = FEvaTelemetrySnapshot();
}

float UEvaPlayerTelemetryComponent::GetHeadshotRate() const
{
    return Telemetry.HitCount > 0 ? static_cast<float>(Telemetry.HeadshotCount) / Telemetry.HitCount : 0.0f;
}

float UEvaPlayerTelemetryComponent::GetAccuracy() const
{
    return Telemetry.ShotCount > 0 ? static_cast<float>(Telemetry.HitCount) / Telemetry.ShotCount : 0.0f;
}

FName UEvaPlayerTelemetryComponent::GetDominantWeapon() const
{
    FName Dominant = NAME_None;
    int32 HighestCount = 0;
    for (const TPair<FName, int32>& Entry : Telemetry.WeaponUseCount)
    {
        if (Entry.Value > HighestCount)
        {
            Dominant = Entry.Key;
            HighestCount = Entry.Value;
        }
    }
    return Dominant;
}

EEvaCombatStyle UEvaPlayerTelemetryComponent::ClassifyCombatStyle() const
{
    if (!Telemetry.LastUsedHideSpot.IsNone())
    {
        return EEvaCombatStyle::Ghost;
    }
    if (!Telemetry.LastKnownEscapeRoute.IsNone())
    {
        return EEvaCombatStyle::Explorer;
    }
    if (Telemetry.CombatDistanceSamples <= 0)
    {
        return EEvaCombatStyle::Unknown;
    }

    if (GetDominantWeapon() == FName(TEXT("Shotgun")) || Telemetry.AverageCombatDistance < 500.0f)
    {
        return EEvaCombatStyle::Berserker;
    }
    if (Telemetry.AverageCombatDistance > 1500.0f)
    {
        return EEvaCombatStyle::Ranger;
    }
    return EEvaCombatStyle::Tactician;
}

UEvaLearningSubsystem* UEvaPlayerTelemetryComponent::GetLearningSubsystem() const
{
    const AActor* Owner = GetOwner();
    const UGameInstance* GameInstance = Owner && Owner->GetWorld() ? Owner->GetWorld()->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UEvaLearningSubsystem>() : nullptr;
}
