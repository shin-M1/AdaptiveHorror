#include "AI/EvaHunterAIController.h"
#include "AdaptiveHorror.h"
#include "AI/EvaHunterCharacter.h"
#include "AI/EvaLearningSubsystem.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AEvaHunterAIController::AEvaHunterAIController()
{
    PrimaryActorTick.TickInterval = 0.10f;
    ConfigureCombat(240.0f, 18.0f, 1.2f);
    ConfigurePerception(2600.0f, 2200.0f);
}

void AEvaHunterAIController::Tick(const float DeltaSeconds)
{
    AAIController::Tick(DeltaSeconds);

    if (!GetPawn() || !GetWorld())
    {
        return;
    }

    if (!TargetActor)
    {
        if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
        {
            SetPlayerTarget(PlayerController->GetPawn());
        }
    }

    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(TargetActor);
    if (!Player || Player->IsDead())
    {
        TargetActor = nullptr;
        ClearFocus(EAIFocusPriority::Gameplay);
        StopMovement();
        return;
    }

    if (!bCounterProfileLocked)
    {
        InitializeCounterProfile();
    }

    ObserveTarget();

    EEvaCombatStyle ObservedStyle = EEvaCombatStyle::Unknown;
    if (const UEvaPlayerTelemetryComponent* Telemetry = Player->GetTelemetryComponent())
    {
        ObservedStyle = Telemetry->ClassifyCombatStyle();
    }
    const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    const UEvaLearningSubsystem* Learning = GameInstance ? GameInstance->GetSubsystem<UEvaLearningSubsystem>() : nullptr;
    const EEvaCombatStyle LearnedStyle = Learning ? Learning->ClassifyAggregateCombatStyle() : EEvaCombatStyle::Unknown;
    const EEvaCombatStyle LockedStyle = CounterTypeToCombatStyle(LockedCounterType);
    if (LockedStyle != EEvaCombatStyle::Unknown)
    {
        ObservedStyle = LockedStyle;
    }
    else if (LearnedStyle == EEvaCombatStyle::Ghost || LearnedStyle == EEvaCombatStyle::Explorer ||
        ObservedStyle == EEvaCombatStyle::Unknown)
    {
        ObservedStyle = LearnedStyle;
    }

    if (CanAttackTarget())
    {
        StopMovement();
        SetFocus(TargetActor);
        TryAttackTarget();
        return;
    }

    SetFocus(TargetActor);
    ExecuteCounterBehavior(ObservedStyle);
}

void AEvaHunterAIController::ObserveTarget()
{
    if (!GetWorld() || !GetPawn() || !TargetActor)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastObservationTime < ObservationInterval)
    {
        return;
    }
    LastObservationTime = Now;

    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(TargetActor);
    if (!Player)
    {
        return;
    }

    EEvaCombatStyle ObservedStyle = EEvaCombatStyle::Unknown;
    FEvaTelemetrySnapshot ObservedSnapshot;
    if (const UEvaPlayerTelemetryComponent* Telemetry = Player->GetTelemetryComponent())
    {
        ObservedStyle = Telemetry->ClassifyCombatStyle();
        ObservedSnapshot = Telemetry->GetTelemetry();
    }

    const FVector PlayerLocation = Player->GetActorLocation();
    const FName EscapeRouteId = ResolveNearbyTaggedActorId(TEXT("EvaEscapeRoute"), PlayerLocation, 650.0f);
    const FName HideSpotId = ResolveNearbyTaggedActorId(TEXT("EvaHideSpot"), PlayerLocation, 650.0f);
    const float DistanceToPlayer = FVector::Distance(GetPawn()->GetActorLocation(), PlayerLocation);

    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->RecordHunterTelemetrySnapshot(ObservedSnapshot);
            Learning->RecordHunterObservation(ObservedStyle, DistanceToPlayer, EscapeRouteId, HideSpotId);
        }
    }
}

void AEvaHunterAIController::InitializeCounterProfile()
{
    AEvaHunterCharacter* Hunter = Cast<AEvaHunterCharacter>(GetPawn());
    if (!Hunter || !GetWorld())
    {
        return;
    }

    const int32 Tier = FMath::Max(1, Hunter->HunterTier);
    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    UEvaLearningSubsystem* Learning = GameInstance ? GameInstance->GetSubsystem<UEvaLearningSubsystem>() : nullptr;
    if (Learning)
    {
        Learning->UpdateAdaptationProfile(false);
        LockedCounterType = Learning->GetHunterCounterTypeForTier(Tier);
    }
    if (LockedCounterType == EEvaHunterCounterType::None)
    {
        LockedCounterType = EEvaHunterCounterType::AntiBerserker;
    }

    const float TierAlpha = FMath::Clamp(static_cast<float>(Tier - 1) * 0.06f, 0.0f, 0.24f);
    float HunterSpeed = 430.0f + TierAlpha * 140.0f;
    float NewRange = 240.0f;
    float NewDamage = 18.0f;
    float NewInterval = 1.20f;

    switch (LockedCounterType)
    {
    case EEvaHunterCounterType::AntiBerserker:
        NewRange = 280.0f;
        NewInterval = FMath::Clamp(1.15f - TierAlpha, 0.82f, 1.20f);
        HunterSpeed += 15.0f;
        break;
    case EEvaHunterCounterType::AntiRanger:
        NewRange = 230.0f;
        NewInterval = FMath::Clamp(1.10f - TierAlpha * 0.8f, 0.82f, 1.20f);
        HunterSpeed += 55.0f;
        break;
    case EEvaHunterCounterType::AntiGhost:
        NewRange = 250.0f;
        ConfigurePerception(3100.0f, 2600.0f);
        HunterSpeed += 30.0f;
        break;
    case EEvaHunterCounterType::AntiExplorer:
        NewRange = 260.0f;
        ConfigurePerception(2850.0f, 2350.0f);
        HunterSpeed += 25.0f;
        break;
    default:
        break;
    }

    ConfigureCombat(NewRange, NewDamage, NewInterval);
    if (UCharacterMovementComponent* MovementComponent = Hunter->GetCharacterMovement())
    {
        MovementComponent->MaxWalkSpeed = FMath::Clamp(HunterSpeed, 420.0f, 540.0f);
    }
    Hunter->SetHunterCounterType(LockedCounterType);
    bCounterProfileLocked = true;

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[HunterAdapt] Locked Pawn=%s Tier=%d Counter=%s Speed=%.1f Range=%.1f Damage=%.1f Interval=%.2f"),
        *Hunter->GetName(),
        Tier,
        *UEnum::GetValueAsString(LockedCounterType),
        HunterSpeed,
        NewRange,
        NewDamage,
        NewInterval);
}

void AEvaHunterAIController::ExecuteCounterBehavior(const EEvaCombatStyle ObservedStyle)
{
    if (!GetWorld() || !GetPawn() || !TargetActor)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastCounterMoveTime < 0.75f)
    {
        return;
    }
    LastCounterMoveTime = Now;

    const FVector PawnLocation = GetPawn()->GetActorLocation();
    const FVector TargetLocation = TargetActor->GetActorLocation();

    switch (ObservedStyle)
    {
    case EEvaCombatStyle::Berserker:
        if (FVector::DistSquared(PawnLocation, TargetLocation) < FMath::Square(PreferredBerserkerCounterRange))
        {
            const FVector AwayDirection = (PawnLocation - TargetLocation).GetSafeNormal2D();
            const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, AwayDirection).GetSafeNormal2D();
            const FVector Side = FMath::RandBool() ? RightDirection : -RightDirection;
            MoveToLocationOrDirect(PawnLocation + (AwayDirection * 520.0f + Side * 320.0f), 90.0f);
            return;
        }
        MoveToActorOrDirect(TargetActor, PreferredBerserkerCounterRange * 0.75f);
        return;
    case EEvaCombatStyle::Ranger:
    {
        const FVector ToTarget = (TargetLocation - PawnLocation).GetSafeNormal2D();
        const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, ToTarget).GetSafeNormal2D();
        const FVector Side = FMath::RandBool() ? RightDirection : -RightDirection;
        if (!Side.IsNearlyZero())
        {
            MoveToLocationOrDirect(PawnLocation + ToTarget * 260.0f + Side * 420.0f, 80.0f);
            return;
        }
        if (AActor* Cover = FindNearestTaggedActor(TEXT("EvaCover"), PawnLocation))
        {
            MoveToLocationOrDirect(Cover->GetActorLocation(), 90.0f);
            return;
        }
        break;
    }
    case EEvaCombatStyle::Ghost:
        if (AActor* HideSpot = FindNearestTaggedActor(TEXT("EvaHideSpot"), TargetLocation))
        {
            MoveToLocationOrDirect(HideSpot->GetActorLocation(), 100.0f);
            return;
        }
        break;
    case EEvaCombatStyle::Explorer:
        if (AActor* AmbushPoint = FindNearestTaggedActor(TEXT("EvaAmbushPoint"), TargetLocation))
        {
            MoveToLocationOrDirect(AmbushPoint->GetActorLocation(), 100.0f);
            return;
        }
        break;
    default:
        break;
    }

    MoveToActorOrDirect(TargetActor, AttackRange * 0.75f);
}

EEvaCombatStyle AEvaHunterAIController::CounterTypeToCombatStyle(const EEvaHunterCounterType CounterType) const
{
    switch (CounterType)
    {
    case EEvaHunterCounterType::AntiBerserker:
        return EEvaCombatStyle::Berserker;
    case EEvaHunterCounterType::AntiRanger:
        return EEvaCombatStyle::Ranger;
    case EEvaHunterCounterType::AntiGhost:
        return EEvaCombatStyle::Ghost;
    case EEvaHunterCounterType::AntiExplorer:
        return EEvaCombatStyle::Explorer;
    default:
        return EEvaCombatStyle::Unknown;
    }
}

FName AEvaHunterAIController::ResolveNearbyTaggedActorId(const FName Tag, const FVector& FromLocation,
    const float MaxDistance) const
{
    if (AActor* Candidate = FindNearestTaggedActor(Tag, FromLocation))
    {
        if (FVector::DistSquared(Candidate->GetActorLocation(), FromLocation) <= FMath::Square(MaxDistance))
        {
            return Candidate->GetFName();
        }
    }
    return NAME_None;
}
