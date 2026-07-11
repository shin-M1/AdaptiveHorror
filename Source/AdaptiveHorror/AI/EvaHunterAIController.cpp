#include "AI/EvaHunterAIController.h"
#include "AI/EvaLearningSubsystem.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
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

    ObserveTarget();

    EEvaCombatStyle ObservedStyle = EEvaCombatStyle::Unknown;
    if (const UEvaPlayerTelemetryComponent* Telemetry = Player->GetTelemetryComponent())
    {
        ObservedStyle = Telemetry->ClassifyCombatStyle();
    }
    const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    const UEvaLearningSubsystem* Learning = GameInstance ? GameInstance->GetSubsystem<UEvaLearningSubsystem>() : nullptr;
    const EEvaCombatStyle LearnedStyle = Learning ? Learning->ClassifyAggregateCombatStyle() : EEvaCombatStyle::Unknown;
    if (LearnedStyle == EEvaCombatStyle::Ghost || LearnedStyle == EEvaCombatStyle::Explorer ||
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
            MoveToLocationOrDirect(PawnLocation + AwayDirection * 700.0f, 90.0f);
            return;
        }
        MoveToActorOrDirect(TargetActor, PreferredBerserkerCounterRange * 0.75f);
        return;
    case EEvaCombatStyle::Ranger:
        if (AActor* Cover = FindNearestTaggedActor(TEXT("EvaCover"), PawnLocation))
        {
            MoveToLocationOrDirect(Cover->GetActorLocation(), 90.0f);
            return;
        }
        break;
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
