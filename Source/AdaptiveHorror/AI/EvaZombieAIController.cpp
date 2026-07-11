#include "AI/EvaZombieAIController.h"
#include "AdaptiveHorror.h"
#include "AI/EvaLearningSubsystem.h"
#include "AI/EvaZombieCharacter.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/EvaHealthComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"

AEvaZombieAIController::AEvaZombieAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.15f;

    EvaPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("EvaPerceptionComponent"));
    SetPerceptionComponent(*EvaPerceptionComponent);

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 1500.0f;
    SightConfig->LoseSightRadius = 1800.0f;
    SightConfig->PeripheralVisionAngleDegrees = 75.0f;
    SightConfig->SetMaxAge(4.0f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = 1000.0f;
    HearingConfig->SetMaxAge(3.0f);
    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

    EvaPerceptionComponent->ConfigureSense(*SightConfig);
    EvaPerceptionComponent->ConfigureSense(*HearingConfig);
    EvaPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
    EvaPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEvaZombieAIController::HandleTargetPerceptionUpdated);
}

void AEvaZombieAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    SetActorTickEnabled(true);
    if (AEvaZombieCharacter* Zombie = Cast<AEvaZombieCharacter>(InPawn))
    {
        Zombie->ApplyEvolutionToController();
    }
}

void AEvaZombieAIController::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(TargetActor);
    if (!Player || Player->IsDead() || !GetPawn())
    {
        ClearPlayerTarget();
        return;
    }

    const FVector CurrentPawnLocation = GetPawn()->GetActorLocation();
    bool bPerformedStuckRecovery = false;
    if (LastObservedPawnLocation.IsNearlyZero())
    {
        LastObservedPawnLocation = CurrentPawnLocation;
    }
    const bool bTryingToReachTarget =
        FVector::DistSquared(CurrentPawnLocation, TargetActor->GetActorLocation()) > FMath::Square(AttackRange * 1.25f);
    if (bTryingToReachTarget && FVector::DistSquared(CurrentPawnLocation, LastObservedPawnLocation) < FMath::Square(8.0f))
    {
        TimeSinceMeaningfulMovement += DeltaSeconds;
        if (TimeSinceMeaningfulMovement > 3.0f)
        {
            if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
            {
                GameMode->NotifyEnemyStuck(GetPawn()->GetName());
            }
            UE_LOG(LogAdaptiveHorror, Warning,
                TEXT("[AI] %s is stuck and will recalculate movement. Target=%s Distance=%.1f MoveFailures=%d"),
                *GetPawn()->GetName(),
                TargetActor ? *TargetActor->GetName() : TEXT("None"),
                TargetActor ? FVector::Dist(CurrentPawnLocation, TargetActor->GetActorLocation()) : -1.0f,
                ConsecutiveMoveFailures);
            StopMovement();
            LastMoveRequestTime = -1000.0f;
            if (TargetActor)
            {
                bPerformedStuckRecovery = TrySidestepAroundObstacle(TargetActor->GetActorLocation());
            }
            TimeSinceMeaningfulMovement = 0.0f;
            LastObservedPawnLocation = CurrentPawnLocation;
        }
    }
    else
    {
        TimeSinceMeaningfulMovement = 0.0f;
        LastObservedPawnLocation = CurrentPawnLocation;
    }

    if (CanAttackTarget())
    {
        StopMovement();
        SetFocus(TargetActor);
        TryAttackTarget();
    }
    else if (TryHandleLearningAdaptation())
    {
        SetFocus(TargetActor);
    }
    else if (bPerformedStuckRecovery)
    {
        SetFocus(TargetActor);
    }
    else
    {
        SetFocus(TargetActor);
        MoveToActorOrDirect(TargetActor, AttackRange * 0.75f);
    }
}

void AEvaZombieAIController::SetPlayerTarget(AActor* NewTarget)
{
    AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(NewTarget);
    if (Player && !Player->IsDead())
    {
        TargetActor = NewTarget;
        UE_LOG(LogAdaptiveHorror, Log, TEXT("[AI] Target set Controller=%s Pawn=%s Target=%s Distance=%.1f"),
            *GetName(),
            GetPawn() ? *GetPawn()->GetName() : TEXT("None"),
            *NewTarget->GetName(),
            GetPawn() ? FVector::Dist(GetPawn()->GetActorLocation(), NewTarget->GetActorLocation()) : -1.0f);
        if (GetPawn())
        {
            MoveToActorOrDirect(TargetActor, AttackRange * 0.75f);
        }
    }
    else
    {
        ClearPlayerTarget();
    }
}

void AEvaZombieAIController::ClearPlayerTarget()
{
    TargetActor = nullptr;
    ClearFocus(EAIFocusPriority::Gameplay);
    StopMovement();
}

void AEvaZombieAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Cast<AEvaPlayerCharacter>(Actor))
    {
        return;
    }

    if (Stimulus.WasSuccessfullySensed())
    {
        SetPlayerTarget(Actor);
    }
    else if (TargetActor == Actor)
    {
        TargetActor = nullptr;
        ClearFocus(EAIFocusPriority::Gameplay);
    }
}

void AEvaZombieAIController::ConfigureCombat(const float NewAttackRange, const float NewAttackDamage,
    const float NewAttackInterval)
{
    AttackRange = FMath::Max(50.0f, NewAttackRange);
    AttackDamage = FMath::Max(1.0f, NewAttackDamage);
    AttackInterval = FMath::Max(0.1f, NewAttackInterval);
}

void AEvaZombieAIController::ConfigurePerception(const float NewSightRadius, const float NewHearingRange)
{
    if (SightConfig)
    {
        SightConfig->SightRadius = FMath::Max(100.0f, NewSightRadius);
        SightConfig->LoseSightRadius = SightConfig->SightRadius + 300.0f;
    }
    if (HearingConfig)
    {
        HearingConfig->HearingRange = FMath::Max(100.0f, NewHearingRange);
    }
    if (EvaPerceptionComponent)
    {
        EvaPerceptionComponent->RequestStimuliListenerUpdate();
    }
}

bool AEvaZombieAIController::CanAttackTarget() const
{
    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(TargetActor);
    return GetPawn() && Player && !Player->IsDead() &&
        FVector::DistSquared(GetPawn()->GetActorLocation(), TargetActor->GetActorLocation()) <= FMath::Square(AttackRange);
}

void AEvaZombieAIController::TryAttackTarget()
{
    if (!GetWorld() || !TargetActor)
    {
        return;
    }

    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(TargetActor);
    if (!Player || Player->IsDead())
    {
        ClearPlayerTarget();
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastAttackTime < AttackInterval)
    {
        return;
    }

    LastAttackTime = Now;
    UGameplayStatics::ApplyDamage(TargetActor, AttackDamage, this, GetPawn(), UDamageType::StaticClass());
}

bool AEvaZombieAIController::TryHandleLearningAdaptation()
{
    if (!GetWorld() || !GetPawn() || !TargetActor)
    {
        return false;
    }

    const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    const UEvaLearningSubsystem* Learning = GameInstance ? GameInstance->GetSubsystem<UEvaLearningSubsystem>() : nullptr;
    if (!Learning)
    {
        return false;
    }

    const EEvaAdaptationDirective Directive = Learning->GetAdaptationDirective();
    if (Directive == EEvaAdaptationDirective::None)
    {
        if (LastAppliedDirective != Directive)
        {
            ConfigurePerception(1500.0f, 1000.0f);
            LastAppliedDirective = Directive;
        }
        return false;
    }

    ApplyAdaptivePerception();

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastAdaptiveMoveTime < 1.0f)
    {
        return false;
    }

    const FVector PawnLocation = GetPawn()->GetActorLocation();
    const FVector TargetLocation = TargetActor->GetActorLocation();
    LastAdaptiveMoveTime = Now;

    switch (Directive)
    {
    case EEvaAdaptationDirective::CounterCloseRange:
        if (FVector::DistSquared(PawnLocation, TargetLocation) < FMath::Square(650.0f))
        {
            const FVector AwayDirection = (PawnLocation - TargetLocation).GetSafeNormal2D();
            MoveToLocationOrDirect(PawnLocation + AwayDirection * 550.0f, 50.0f);
            return true;
        }
        break;
    case EEvaAdaptationDirective::CounterLongRange:
        if (AActor* Cover = FindNearestTaggedActor(TEXT("EvaCover"), PawnLocation))
        {
            MoveToLocationOrDirect(Cover->GetActorLocation(), 90.0f);
            return true;
        }
        break;
    case EEvaAdaptationDirective::CounterStealth:
        MoveToActorOrDirect(TargetActor, AttackRange * 0.75f);
        return true;
    case EEvaAdaptationDirective::CounterExplorer:
        if (AActor* AmbushPoint = FindNearestTaggedActor(TEXT("EvaAmbushPoint"), TargetLocation))
        {
            MoveToLocationOrDirect(AmbushPoint->GetActorLocation(), 100.0f);
            return true;
        }
        break;
    default:
        break;
    }

    return false;
}

void AEvaZombieAIController::ApplyAdaptivePerception()
{
    if (!GetWorld())
    {
        return;
    }

    const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    const UEvaLearningSubsystem* Learning = GameInstance ? GameInstance->GetSubsystem<UEvaLearningSubsystem>() : nullptr;
    const EEvaAdaptationDirective Directive = Learning ? Learning->GetAdaptationDirective() : EEvaAdaptationDirective::None;
    if (Directive == LastAppliedDirective)
    {
        return;
    }

    if (Directive == EEvaAdaptationDirective::CounterStealth)
    {
        ConfigurePerception(2300.0f, 1800.0f);
    }
    else
    {
        ConfigurePerception(1500.0f, 1000.0f);
    }
    LastAppliedDirective = Directive;
}

bool AEvaZombieAIController::MoveToActorOrDirect(AActor* GoalActor, const float AcceptanceRadius)
{
    if (!GoalActor || !GetPawn())
    {
        return false;
    }

    UNavigationPath* DiagnosticPath = GetWorld() ?
        UNavigationSystemV1::FindPathToActorSynchronously(GetWorld(), GetPawn()->GetActorLocation(),
            GoalActor, 50.0f, GetPawn()) :
        nullptr;
    const bool bHasValidDiagnosticPath = DiagnosticPath && DiagnosticPath->IsValid();
    const int32 PathPointCount = DiagnosticPath ? DiagnosticPath->PathPoints.Num() : 0;

    const EPathFollowingRequestResult::Type MoveResult =
        MoveToActor(GoalActor, AcceptanceRadius, true, true, true, nullptr, true);
    if (MoveResult != EPathFollowingRequestResult::Failed)
    {
        ConsecutiveMoveFailures = 0;
        const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        if (Now - LastMoveDiagnosticLogTime > 1.25f)
        {
            LastMoveDiagnosticLogTime = Now;
            UE_LOG(LogAdaptiveHorror, Log,
                TEXT("[AI] MoveToActor accepted Controller=%s Pawn=%s GoalActor=%s Acceptance=%.1f PathValid=%s PathPoints=%d Result=%d"),
                *GetName(),
                GetPawn() ? *GetPawn()->GetName() : TEXT("None"),
                *GoalActor->GetName(),
                AcceptanceRadius,
                bHasValidDiagnosticPath ? TEXT("true") : TEXT("false"),
                PathPointCount,
                static_cast<int32>(MoveResult));
        }
        return true;
    }

    ++ConsecutiveMoveFailures;
    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[AI] MoveToActor failed Controller=%s Pawn=%s GoalActor=%s Acceptance=%.1f PathValid=%s PathPoints=%d Failures=%d"),
        *GetName(),
        GetPawn() ? *GetPawn()->GetName() : TEXT("None"),
        *GoalActor->GetName(),
        AcceptanceRadius,
        bHasValidDiagnosticPath ? TEXT("true") : TEXT("false"),
        PathPointCount,
        ConsecutiveMoveFailures);
    return MoveToLocationOrDirect(GoalActor->GetActorLocation(), AcceptanceRadius);
}

bool AEvaZombieAIController::MoveToLocationOrDirect(const FVector& GoalLocation, const float AcceptanceRadius)
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !GetWorld())
    {
        return false;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (GetMoveStatus() == EPathFollowingStatus::Moving &&
        FVector::DistSquared2D(LastMoveRequestGoal, GoalLocation) < FMath::Square(90.0f) &&
        Now - LastMoveRequestTime < 0.55f)
    {
        return true;
    }

    LastMoveRequestGoal = GoalLocation;
    LastMoveRequestTime = Now;

    UNavigationPath* DiagnosticPath = GetWorld() ?
        UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), ControlledPawn->GetActorLocation(),
            GoalLocation, ControlledPawn) :
        nullptr;
    const bool bHasValidDiagnosticPath = DiagnosticPath && DiagnosticPath->IsValid();
    const int32 PathPointCount = DiagnosticPath ? DiagnosticPath->PathPoints.Num() : 0;

    const EPathFollowingRequestResult::Type MoveResult =
        MoveToLocation(GoalLocation, AcceptanceRadius, true, true, true, true, nullptr, true);
    if (MoveResult != EPathFollowingRequestResult::Failed)
    {
        ConsecutiveMoveFailures = 0;
        if (Now - LastMoveDiagnosticLogTime > 1.25f)
        {
            LastMoveDiagnosticLogTime = Now;
            UE_LOG(LogAdaptiveHorror, Log,
                TEXT("[AI] MoveToLocation accepted Controller=%s Pawn=%s Goal=%s Acceptance=%.1f PathValid=%s PathPoints=%d Result=%d"),
                *GetName(),
                *ControlledPawn->GetName(),
                *GoalLocation.ToCompactString(),
                AcceptanceRadius,
                bHasValidDiagnosticPath ? TEXT("true") : TEXT("false"),
                PathPointCount,
                static_cast<int32>(MoveResult));
        }
        return true;
    }

    ++ConsecutiveMoveFailures;
    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[AI] MoveToLocation failed Controller=%s Pawn=%s Goal=%s Acceptance=%.1f PathValid=%s PathPoints=%d Failures=%d"),
        *GetName(),
        *ControlledPawn->GetName(),
        *GoalLocation.ToCompactString(),
        AcceptanceRadius,
        bHasValidDiagnosticPath ? TEXT("true") : TEXT("false"),
        PathPointCount,
        ConsecutiveMoveFailures);

    const FVector DirectionToGoal = (GoalLocation - ControlledPawn->GetActorLocation()).GetSafeNormal2D();
    if (DirectionToGoal.IsNearlyZero())
    {
        return false;
    }

    FVector SeparationDirection = FVector::ZeroVector;
    for (TActorIterator<AEvaZombieCharacter> It(GetWorld()); It; ++It)
    {
        const AEvaZombieCharacter* OtherEnemy = *It;
        if (!OtherEnemy || OtherEnemy == Cast<AEvaZombieCharacter>(ControlledPawn) || !OtherEnemy->GetHealthComponent() ||
            OtherEnemy->GetHealthComponent()->IsDead())
        {
            continue;
        }

        const FVector Away = ControlledPawn->GetActorLocation() - OtherEnemy->GetActorLocation();
        const float Distance = Away.Size2D();
        if (Distance > KINDA_SMALL_NUMBER && Distance < 220.0f)
        {
            SeparationDirection += Away.GetSafeNormal2D() * ((220.0f - Distance) / 220.0f);
        }
    }

    const FVector MoveDirection = (DirectionToGoal * 0.82f + SeparationDirection.GetClampedToMaxSize(1.0f) * 0.45f)
        .GetSafeNormal2D();
    if (MoveDirection.IsNearlyZero())
    {
        return false;
    }

    FHitResult WallHit;
    const FVector TraceStart = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
    const FVector TraceEnd = TraceStart + MoveDirection * 95.0f;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EvaDirectMoveObstacle), false, ControlledPawn);
    const bool bBlockedAhead = GetWorld()->LineTraceSingleByChannel(WallHit, TraceStart, TraceEnd,
        ECC_WorldStatic, QueryParams);
    if (bBlockedAhead)
    {
#if !UE_BUILD_SHIPPING
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 0.35f, 0, 2.0f);
#endif
        return TrySidestepAroundObstacle(GoalLocation);
    }

    // PIE runtime graybox may not have a baked NavMesh yet. Direct movement keeps the vertical slice playable
    // instead of leaving enemies idle until a proper authored map/nav volume replaces the runtime prototype.
    return ApplyDirectFallbackMovement(MoveDirection, FColor::Yellow);
}

bool AEvaZombieAIController::TrySidestepAroundObstacle(const FVector& GoalLocation)
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !GetWorld())
    {
        return false;
    }

    const FVector DirectionToGoal = (GoalLocation - ControlledPawn->GetActorLocation()).GetSafeNormal2D();
    if (DirectionToGoal.IsNearlyZero())
    {
        return false;
    }

    const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, DirectionToGoal).GetSafeNormal2D();
    const FVector FirstSide = bPreferRightDetour ? RightDirection : -RightDirection;
    const FVector SecondSide = -FirstSide;
    bPreferRightDetour = !bPreferRightDetour;

    const FVector CandidateDirections[] =
    {
        (DirectionToGoal * 0.30f + FirstSide).GetSafeNormal2D(),
        (DirectionToGoal * 0.30f + SecondSide).GetSafeNormal2D(),
        FirstSide,
        SecondSide,
        (DirectionToGoal * -0.20f + FirstSide).GetSafeNormal2D(),
        (DirectionToGoal * -0.20f + SecondSide).GetSafeNormal2D()
    };

    for (const FVector& CandidateDirection : CandidateDirections)
    {
        if (ApplyDirectFallbackMovement(CandidateDirection, FColor::Cyan))
        {
            UE_LOG(LogAdaptiveHorror, Log,
                TEXT("[AI] Sidestep detour Controller=%s Pawn=%s Goal=%s Direction=%s"),
                *GetName(),
                *ControlledPawn->GetName(),
                *GoalLocation.ToCompactString(),
                *CandidateDirection.ToCompactString());
            return true;
        }
    }

    return false;
}

bool AEvaZombieAIController::ApplyDirectFallbackMovement(const FVector& DesiredDirection, const FColor& DebugColor)
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !GetWorld())
    {
        return false;
    }

    const FVector MoveDirection = DesiredDirection.GetSafeNormal2D();
    if (MoveDirection.IsNearlyZero())
    {
        return false;
    }

    FHitResult WallHit;
    const FVector TraceStart = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
    const FVector TraceEnd = TraceStart + MoveDirection * 115.0f;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EvaDirectMoveFallback), false, ControlledPawn);
    const bool bBlockedAhead = GetWorld()->LineTraceSingleByChannel(WallHit, TraceStart, TraceEnd,
        ECC_WorldStatic, QueryParams);
    if (bBlockedAhead)
    {
#if !UE_BUILD_SHIPPING
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 0.35f, 0, 2.0f);
#endif
        return false;
    }

    ControlledPawn->AddMovementInput(MoveDirection, 1.0f);
    if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
    {
        GameMode->NotifyFallbackMovementUsed();
    }
#if !UE_BUILD_SHIPPING
    DrawDebugLine(GetWorld(), ControlledPawn->GetActorLocation(),
        ControlledPawn->GetActorLocation() + MoveDirection * 160.0f, DebugColor, false, 0.35f, 0, 2.0f);
#endif
    return true;
}

AActor* AEvaZombieAIController::FindNearestTaggedActor(const FName Tag, const FVector& FromLocation) const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    TArray<AActor*> TaggedActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), Tag, TaggedActors);

    AActor* BestActor = nullptr;
    float BestDistanceSq = TNumericLimits<float>::Max();
    for (AActor* Candidate : TaggedActors)
    {
        if (!Candidate)
        {
            continue;
        }
        const float DistanceSq = FVector::DistSquared(FromLocation, Candidate->GetActorLocation());
        if (DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            BestActor = Candidate;
        }
    }
    return BestActor;
}
