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
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "NavigationPath.h"
#include "NavigationData.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"

namespace
{
const TCHAR* EvaMoveRequestResultToString(const EPathFollowingRequestResult::Type Result)
{
    switch (Result)
    {
    case EPathFollowingRequestResult::Failed:
        return TEXT("Failed");
    case EPathFollowingRequestResult::AlreadyAtGoal:
        return TEXT("AlreadyAtGoal");
    case EPathFollowingRequestResult::RequestSuccessful:
        return TEXT("RequestSuccessful");
    default:
        return TEXT("Unknown");
    }
}

const TCHAR* EvaPathStatusToString(const EPathFollowingStatus::Type Status)
{
    switch (Status)
    {
    case EPathFollowingStatus::Idle:
        return TEXT("Idle");
    case EPathFollowingStatus::Waiting:
        return TEXT("Waiting");
    case EPathFollowingStatus::Paused:
        return TEXT("Paused");
    case EPathFollowingStatus::Moving:
        return TEXT("Moving");
    default:
        return TEXT("Unknown");
    }
}

const TCHAR* EvaPathResultCodeToString(const EPathFollowingResult::Type ResultCode)
{
    switch (ResultCode)
    {
    case EPathFollowingResult::Success:
        return TEXT("Success");
    case EPathFollowingResult::Blocked:
        return TEXT("Blocked");
    case EPathFollowingResult::OffPath:
        return TEXT("OffPath");
    case EPathFollowingResult::Aborted:
        return TEXT("Aborted");
    case EPathFollowingResult::Invalid:
        return TEXT("Invalid");
    default:
        return TEXT("Unknown");
    }
}
}

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
    LastObservedPawnLocation = InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector;
    LastProgressSampleLocation = LastObservedPawnLocation;
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    LastProgressSampleTime = Now;
    LastMeaningfulProgressTime = Now;
    if (AEvaZombieCharacter* Zombie = Cast<AEvaZombieCharacter>(InPawn))
    {
        Zombie->ApplyEvolutionToController();
    }
    if (const ACharacter* ControlledCharacter = Cast<ACharacter>(InPawn))
    {
        if (const UCharacterMovementComponent* MovementComponent = ControlledCharacter->GetCharacterMovement())
        {
            BaseConfiguredMoveSpeed = MovementComponent->MaxWalkSpeed;
        }
    }
    ApplyCurrentGameplayAdaptation(true);
}

void AEvaZombieAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);

    if (!bCombatEnabled)
    {
        return;
    }

    const UPathFollowingComponent* PathComponent = GetPathFollowingComponent();
    const FNavPathSharedPtr ActivePath = PathComponent ? PathComponent->GetPath() : nullptr;
    const int32 ActivePathPoints = ActivePath.IsValid() ? ActivePath->GetPathPoints().Num() : 0;
    const bool bPathValid = PathComponent && PathComponent->HasValidPath();
    const bool bPartial = PathComponent && PathComponent->HasPartialPath();

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[AIPath] MoveCompleted Controller=%s Pawn=%s RequestId=%u ResultCode=%s Result=%s Flags=0x%04x PathStatus=%s PathValid=%s IsPartial=%s PathPoints=%d CurrentPathPointIndex=%d"),
        *GetName(),
        GetPawn() ? *GetPawn()->GetName() : TEXT("None"),
        RequestID.GetID(),
        EvaPathResultCodeToString(Result.Code),
        *Result.ToString(),
        static_cast<uint32>(Result.Flags),
        PathComponent ? EvaPathStatusToString(PathComponent->GetStatus()) : TEXT("NoPathFollowingComponent"),
        bPathValid ? TEXT("true") : TEXT("false"),
        bPartial ? TEXT("true") : TEXT("false"),
        ActivePathPoints,
        PathComponent ? PathComponent->GetCurrentPathIndex() : INDEX_NONE);

    if (bInternalRepathAbort || bIssuingRepathMove)
    {
        LogRepathState(TEXT("MoveCompleted"), EPathFollowingRequestResult::Failed, LastProgressDistance);
        return;
    }

    if (bRecoveringSidestep && (Result.IsSuccess() || Result.Code == EPathFollowingResult::Blocked ||
        Result.Code == EPathFollowingResult::OffPath || Result.Code == EPathFollowingResult::Invalid))
    {
        bRecoveringSidestep = false;
        LastMoveRequestTime = -1000.0f;
        if (TargetActor && GetPawn())
        {
            ReissueMoveToTarget(TEXT("SidestepFinished"), false);
        }
        return;
    }

    if (TargetActor && GetPawn() && Result.Code != EPathFollowingResult::Success &&
        Result.Code != EPathFollowingResult::Aborted)
    {
        ReissueMoveToTarget(TEXT("MoveCompleted"), false);
    }
}

void AEvaZombieAIController::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bCombatEnabled)
    {
        return;
    }

    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(TargetActor);
    if (!Player || Player->IsDead() || !GetPawn())
    {
        ClearPlayerTarget();
        return;
    }

    if (bRecoveringSidestep && GetWorld())
    {
        const float Now = GetWorld()->GetTimeSeconds();
        if (GetMoveStatus() == EPathFollowingStatus::Moving && Now - LastSidestepMoveTime < 1.4f)
        {
            SetFocus(TargetActor);
            return;
        }
        bRecoveringSidestep = false;
        if (ReissueMoveToTarget(TEXT("SidestepFinished"), true))
        {
            SetFocus(TargetActor);
            return;
        }
    }

    const FVector CurrentPawnLocation = GetPawn()->GetActorLocation();
    const float DistanceToTarget = TargetActor ? FVector::Dist(CurrentPawnLocation, TargetActor->GetActorLocation()) : -1.0f;
    const bool bInAttackRange = TargetActor && DistanceToTarget <= AttackRange;
    const bool bAttackLineOfSight = bInAttackRange && HasAttackLineOfSightToTarget();
    if (GetWorld())
    {
        const float Now = GetWorld()->GetTimeSeconds();
        const float AttackDiagInterval = bInAttackRange ? 0.65f : 1.35f;
        if (Now - LastMoveDiagnosticLogTime >= AttackDiagInterval)
        {
            LastMoveDiagnosticLogTime = Now;
            UE_LOG(LogAdaptiveHorror, Log,
                TEXT("[ZombieAttackDiag] Stage=RangeCheck Controller=%s Pawn=%s Target=%s Distance=%.1f AttackRange=%.1f InRange=%s LineOfSight=%s MoveStatus=%s Intent=%s"),
                *GetName(),
                GetPawn() ? *GetPawn()->GetName() : TEXT("None"),
                TargetActor ? *TargetActor->GetName() : TEXT("None"),
                DistanceToTarget,
                AttackRange,
                bInAttackRange ? TEXT("true") : TEXT("false"),
                bAttackLineOfSight ? TEXT("true") : TEXT("false"),
                EvaPathStatusToString(GetMoveStatus()),
                *GetCurrentActionIntent());
        }
    }
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
            LogPathDiagnostics(TEXT("StuckRecovery"), TargetActor ? TargetActor->GetActorLocation() : CurrentPawnLocation,
                EPathFollowingRequestResult::Failed);
            if (TargetActor)
            {
                const UPathFollowingComponent* PathComponent = GetPathFollowingComponent();
                const bool bHasUsableFullPath = PathComponent && PathComponent->HasValidPath() && !PathComponent->HasPartialPath();
                LastMoveRequestTime = -1000.0f;
                if (bHasUsableFullPath)
                {
                    bPerformedStuckRecovery = MoveToActorOrDirect(TargetActor, AttackRange * 0.75f);
                }
                else
                {
                    StopMovement();
                    bPerformedStuckRecovery = TrySidestepAroundObstacle(TargetActor->GetActorLocation());
                }
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
    else if (EvaluateRepathForStationaryTarget(DeltaSeconds))
    {
        SetFocus(TargetActor);
    }
    else
    {
        SetFocus(TargetActor);
        SetCurrentActionIntent(TEXT("CHASE"));
        MoveToActorOrDirect(TargetActor, AttackRange * 0.75f);
    }
}

void AEvaZombieAIController::SetPlayerTarget(AActor* NewTarget)
{
    if (!bCombatEnabled)
    {
        return;
    }

    AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(NewTarget);
    if (Player && !Player->IsDead())
    {
        TargetActor = NewTarget;
        LastRepathTargetLocation = NewTarget->GetActorLocation();
        if (GetPawn())
        {
            LastProgressSampleLocation = GetPawn()->GetActorLocation();
            const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
            LastProgressSampleTime = Now;
            LastMeaningfulProgressTime = Now;
        }
        UE_LOG(LogAdaptiveHorror, Log, TEXT("[AI] Target set Controller=%s Pawn=%s Target=%s Distance=%.1f"),
            *GetName(),
            GetPawn() ? *GetPawn()->GetName() : TEXT("None"),
            *NewTarget->GetName(),
            GetPawn() ? FVector::Dist(GetPawn()->GetActorLocation(), NewTarget->GetActorLocation()) : -1.0f);
        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[ZombieAttackDiag] Stage=TargetAcquired Controller=%s Pawn=%s Target=%s Distance=%.1f AttackRange=%.1f"),
            *GetName(),
            GetPawn() ? *GetPawn()->GetName() : TEXT("None"),
            *NewTarget->GetName(),
            GetPawn() ? FVector::Dist(GetPawn()->GetActorLocation(), NewTarget->GetActorLocation()) : -1.0f,
            AttackRange);
        SetCurrentActionIntent(TEXT("CHASE"));
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
    SetCurrentActionIntent(TEXT("IDLE"));
    ClearFocus(EAIFocusPriority::Gameplay);
    bRecoveringSidestep = false;
    bDirectFallbackActive = false;
    StopMovement();
}

void AEvaZombieAIController::StopCombatForStageClear()
{
    bCombatEnabled = false;
    TargetActor = nullptr;
    SetCurrentActionIntent(TEXT("IDLE"));
    bRecoveringSidestep = false;
    bDirectFallbackActive = false;
    bInternalRepathAbort = true;
    ClearFocus(EAIFocusPriority::Gameplay);
    StopMovement();
    bInternalRepathAbort = false;
    if (EvaPerceptionComponent)
    {
        EvaPerceptionComponent->Deactivate();
    }
    SetActorTickEnabled(false);
}

void AEvaZombieAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!bCombatEnabled)
    {
        return;
    }

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
    BaseConfiguredAttackRange = FMath::Max(50.0f, NewAttackRange);
    BaseConfiguredAttackDamage = FMath::Max(1.0f, NewAttackDamage);
    BaseConfiguredAttackInterval = FMath::Max(0.1f, NewAttackInterval);
    AttackRange = BaseConfiguredAttackRange;
    AttackDamage = BaseConfiguredAttackDamage;
    AttackInterval = BaseConfiguredAttackInterval;
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

bool AEvaZombieAIController::ApplyCurrentGameplayAdaptation(const bool bForceApply)
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !GetWorld())
    {
        return false;
    }

    const AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>();
    if (GameMode && !GameMode->IsGameplayActive())
    {
        return false;
    }

    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    UEvaLearningSubsystem* Learning = GameInstance ? GameInstance->GetSubsystem<UEvaLearningSubsystem>() : nullptr;
    if (!Learning)
    {
        return false;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const FEvaPlayerAdaptationProfile Profile = Learning->UpdateAdaptationProfile(false);
    const EEvaEvolutionType EvolutionType = Cast<AEvaZombieCharacter>(ControlledPawn) ?
        Cast<AEvaZombieCharacter>(ControlledPawn)->GetEvolutionType() : EEvaEvolutionType::None;
    FEvaEnemyAdaptationTuning Tuning = Learning->BuildEnemyAdaptationTuning(EvolutionType);
    if (EvolutionType == EEvaEvolutionType::Composite)
    {
        const float HoldSeconds = FMath::Max(0.0f, bHasLockedCompositeTuning ?
            LockedCompositeTuning.CompositeHybridHoldSeconds : Tuning.CompositeHybridHoldSeconds);
        if (bHasLockedCompositeTuning && Now - LastCompositeHybridLockTime < HoldSeconds)
        {
            Tuning = LockedCompositeTuning;
        }
        else
        {
            LockedCompositeTuning = Tuning;
            LastCompositeHybridLockTime = Now;
            bHasLockedCompositeTuning = true;
        }
    }
    else
    {
        bHasLockedCompositeTuning = false;
    }

    const bool bProfileChanged = LastAdaptationProfile.CombatStyle != Profile.CombatStyle ||
        LastAdaptationProfile.EvaStage != Profile.EvaStage ||
        CurrentAdaptationTuning.BehaviorRole != Tuning.BehaviorRole ||
        CurrentAdaptationTuning.EvolutionType != Tuning.EvolutionType;
    if (!bForceApply && !bProfileChanged && Now - LastAdaptationApplyTime < 3.5f)
    {
        return false;
    }

    LastAdaptationProfile = Profile;
    CurrentAdaptationTuning = Tuning;
    LastAdaptationApplyTime = Now;

    AttackRange = FMath::Clamp(BaseConfiguredAttackRange * Tuning.AttackRangeMultiplier, 60.0f, 520.0f);
    AttackDamage = FMath::Clamp(BaseConfiguredAttackDamage * Tuning.DamageMultiplier, 1.0f, 40.0f);
    AttackInterval = FMath::Clamp(BaseConfiguredAttackInterval * Tuning.AttackCooldownMultiplier, 0.55f, 2.75f);

    if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
    {
        if (UCharacterMovementComponent* MovementComponent = ControlledCharacter->GetCharacterMovement())
        {
            if (BaseConfiguredMoveSpeed <= 0.0f)
            {
                BaseConfiguredMoveSpeed = MovementComponent->MaxWalkSpeed;
            }
            MovementComponent->MaxWalkSpeed = FMath::Clamp(BaseConfiguredMoveSpeed * Tuning.MoveSpeedMultiplier,
                160.0f, 560.0f);
        }
    }

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[EnemyAdapt] Applied Pawn=%s Evolution=%s Role=%s Label=%s Hybrid=%s HybridRoles=%d Counter=%s Style=%s Speed=%.2f Range=%.2f Cooldown=%.2f Damage=%.2f Side=%.2f Search=%.1f Summary=%s"),
        *ControlledPawn->GetName(),
        *UEnum::GetValueAsString(Tuning.EvolutionType),
        *UEnum::GetValueAsString(Tuning.BehaviorRole),
        *Tuning.RoleLabel,
        Tuning.CompositeHybridType.IsEmpty() ? TEXT("None") : *Tuning.CompositeHybridType,
        Tuning.CompositeHybridRoleCount,
        *UEnum::GetValueAsString(Tuning.HunterCounterType),
        *UEnum::GetValueAsString(Profile.CombatStyle),
        Tuning.MoveSpeedMultiplier,
        Tuning.AttackRangeMultiplier,
        Tuning.AttackCooldownMultiplier,
        Tuning.DamageMultiplier,
        Tuning.SidestepChance,
        Tuning.SearchDuration,
        *Tuning.DebugSummary);
    if (CurrentActionIntent.IsEmpty() && !Tuning.IntentLabel.IsEmpty())
    {
        SetCurrentActionIntent(Tuning.IntentLabel);
    }
    return true;
}

FString AEvaZombieAIController::GetCurrentActionIntent() const
{
    return ResolveCurrentActionIntent();
}

void AEvaZombieAIController::EnsureCurrentActionIntent()
{
    SetCurrentActionIntent(ResolveCurrentActionIntent());
}

void AEvaZombieAIController::SetCurrentActionIntent(const FString& NewIntent)
{
    const FString SafeIntent = NewIntent.IsEmpty() ? ResolveCurrentActionIntent() : NewIntent;
    if (CurrentActionIntent == SafeIntent)
    {
        return;
    }

    CurrentActionIntent = SafeIntent;
    if (AEvaZombieCharacter* Zombie = Cast<AEvaZombieCharacter>(GetPawn()))
    {
        Zombie->SetDebugIntentText(CurrentActionIntent);
    }
}

FString AEvaZombieAIController::ResolveCurrentActionIntent() const
{
    if (!bCombatEnabled)
    {
        return TEXT("IDLE");
    }

    if (!CurrentActionIntent.IsEmpty())
    {
        return CurrentActionIntent;
    }

    if (!GetPawn())
    {
        return TEXT("IDLE");
    }

    if (TargetActor)
    {
        const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(TargetActor);
        if (Player && Player->IsDead())
        {
            return TEXT("IDLE");
        }
        return CanAttackTarget() ? FString(TEXT("ATTACK")) : FString(TEXT("CHASE"));
    }

    return GetMoveStatus() == EPathFollowingStatus::Moving ? FString(TEXT("SEARCH")) : FString(TEXT("IDLE"));
}

bool AEvaZombieAIController::CanAttackTarget() const
{
    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(TargetActor);
    if (!bCombatEnabled || !GetPawn() || !Player || Player->IsDead() ||
        FVector::DistSquared(GetPawn()->GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(AttackRange))
    {
        return false;
    }

    return HasAttackLineOfSightToTarget();
}

bool AEvaZombieAIController::HasAttackLineOfSightToTarget() const
{
    if (!GetWorld() || !GetPawn() || !TargetActor)
    {
        return false;
    }

    FHitResult ObstacleHit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EvaEnemyAttackLineOfSight), false, GetPawn());
    QueryParams.AddIgnoredActor(GetPawn());
    const FVector TraceStart = GetPawn()->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
    const FVector TraceEnd = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
    if (GetWorld()->LineTraceSingleByChannel(ObstacleHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
    {
        UE_LOG(LogAdaptiveHorror, Verbose,
            TEXT("[EnemyAttack] BlockedByObstacle Pawn=%s Target=%s Hit=%s Location=%s"),
            GetPawn() ? *GetPawn()->GetName() : TEXT("None"),
            TargetActor ? *TargetActor->GetName() : TEXT("None"),
            ObstacleHit.GetActor() ? *ObstacleHit.GetActor()->GetName() : TEXT("None"),
            *ObstacleHit.ImpactPoint.ToCompactString());
        return false;
    }

    return true;
}

void AEvaZombieAIController::TryAttackTarget()
{
    if (!bCombatEnabled || !GetWorld() || !TargetActor)
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
    if (!CanAttackTarget())
    {
        return;
    }

    LastAttackTime = Now;
    SetCurrentActionIntent(TEXT("ATTACK"));
    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[ZombieAttackDiag] Stage=AttackStateStart Controller=%s Pawn=%s Target=%s Distance=%.1f AttackRange=%.1f Damage=%.1f"),
        *GetName(),
        GetPawn() ? *GetPawn()->GetName() : TEXT("None"),
        TargetActor ? *TargetActor->GetName() : TEXT("None"),
        GetPawn() && TargetActor ? FVector::Dist(GetPawn()->GetActorLocation(), TargetActor->GetActorLocation()) : -1.0f,
        AttackRange,
        AttackDamage);
    UGameplayStatics::ApplyDamage(TargetActor, AttackDamage, this, GetPawn(), UDamageType::StaticClass());
    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[ZombieAttackDiag] Stage=DamageApplied Controller=%s Pawn=%s Target=%s Damage=%.1f"),
        *GetName(),
        GetPawn() ? *GetPawn()->GetName() : TEXT("None"),
        TargetActor ? *TargetActor->GetName() : TEXT("None"),
        AttackDamage);
    if (AEvaZombieCharacter* Zombie = Cast<AEvaZombieCharacter>(GetPawn()))
    {
        Zombie->PlayPrototypeAttackFeedback();
    }

    if (CurrentAdaptationTuning.BehaviorRole == EEvaEnemyBehaviorRole::Flanker ||
        CurrentAdaptationTuning.HunterCounterType == EEvaHunterCounterType::AntiBerserker)
    {
        if (FMath::FRand() <= CurrentAdaptationTuning.SidestepChance)
        {
            const FVector PawnLocation = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
            const FVector TargetLocation = TargetActor ? TargetActor->GetActorLocation() : PawnLocation;
            const FVector AwayDirection = (PawnLocation - TargetLocation).GetSafeNormal2D();
            const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, AwayDirection).GetSafeNormal2D();
            const FVector RetreatDirection = (AwayDirection * 0.65f +
                (FMath::RandBool() ? RightDirection : -RightDirection) * 0.75f).GetSafeNormal2D();
            if (!RetreatDirection.IsNearlyZero())
            {
                if (MoveToLocationOrDirect(PawnLocation + RetreatDirection * 320.0f, 70.0f))
                {
                    SetCurrentActionIntent(TEXT("DISENGAGE"));
                }
            }
        }
    }
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

    ApplyCurrentGameplayAdaptation(false);

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

    if (TryMoveForAdaptationRole(CurrentAdaptationTuning, PawnLocation, TargetLocation))
    {
        return true;
    }

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
    else if (Directive == EEvaAdaptationDirective::CounterExplorer)
    {
        ConfigurePerception(1850.0f, 1350.0f);
    }
    else
    {
        ConfigurePerception(1500.0f, 1000.0f);
    }
    LastAppliedDirective = Directive;
}

bool AEvaZombieAIController::EvaluateRepathForStationaryTarget(const float DeltaSeconds)
{
    (void)DeltaSeconds;

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !TargetActor || !GetWorld())
    {
        return false;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const FVector CurrentLocation = ControlledPawn->GetActorLocation();
    const FVector TargetLocation = TargetActor->GetActorLocation();
    if (LastProgressSampleTime < 0.0f || LastProgressSampleLocation.IsNearlyZero())
    {
        LastProgressSampleLocation = CurrentLocation;
        LastProgressSampleTime = Now;
        LastMeaningfulProgressTime = Now;
        LastRepathTargetLocation = TargetLocation;
        return false;
    }

    const float SampleAge = FMath::Max(0.001f, Now - LastProgressSampleTime);
    const float RecentMoveDistance = FVector::Dist2D(CurrentLocation, LastProgressSampleLocation);
    LastProgressDistance = RecentMoveDistance;
    if (RecentMoveDistance >= 18.0f)
    {
        LastMeaningfulProgressTime = Now;
        LastProgressSampleLocation = CurrentLocation;
        LastProgressSampleTime = Now;
    }
    else if (SampleAge >= 1.0f)
    {
        LastProgressSampleLocation = CurrentLocation;
        LastProgressSampleTime = Now;
    }

    if (Now - LastRepathMonitorTime < 0.65f)
    {
        return false;
    }
    LastRepathMonitorTime = Now;

    const bool bTryingToReachTarget =
        FVector::DistSquared(CurrentLocation, TargetLocation) > FMath::Square(AttackRange * 1.25f);
    if (!bTryingToReachTarget)
    {
        LastRepathTargetLocation = TargetLocation;
        return false;
    }

    UPathFollowingComponent* PathComponent = GetPathFollowingComponent();
    const bool bHasPathComponent = PathComponent != nullptr;
    const EPathFollowingStatus::Type PathStatus = bHasPathComponent ? PathComponent->GetStatus() : EPathFollowingStatus::Idle;
    const bool bHasValidPath = bHasPathComponent && PathComponent->HasValidPath();
    const bool bPathInvalid = !bHasPathComponent || !bHasValidPath || PathStatus == EPathFollowingStatus::Idle;
    const bool bTargetMoved = FVector::DistSquared2D(TargetLocation, LastRepathTargetLocation) > FMath::Square(160.0f);
    const float TimeSinceMoveRequest = Now - LastMoveRequestTime;
    const float TimeSinceMeaningfulProgress = Now - LastMeaningfulProgressTime;
    const bool bNoProgress = PathStatus == EPathFollowingStatus::Moving && bHasValidPath &&
        TimeSinceMeaningfulProgress >= 1.25f && RecentMoveDistance < 12.0f;
    const bool bPeriodicRefresh = PathStatus == EPathFollowingStatus::Moving && bHasValidPath &&
        TimeSinceMoveRequest >= 2.25f && RecentMoveDistance < 24.0f;

    if (bTargetMoved && TimeSinceMoveRequest >= 0.5f)
    {
        LastRepathTargetLocation = TargetLocation;
        return ReissueMoveToTarget(TEXT("TargetMoved"), true);
    }
    if (bPathInvalid && TimeSinceMoveRequest >= 0.5f)
    {
        return ReissueMoveToTarget(TEXT("PathInvalid"), true);
    }
    if (bNoProgress && TimeSinceMoveRequest >= 0.75f)
    {
        return ReissueMoveToTarget(TEXT("NoProgress"), true);
    }
    if (bPeriodicRefresh)
    {
        return ReissueMoveToTarget(TEXT("PeriodicRefresh"), true);
    }

    return false;
}

bool AEvaZombieAIController::TryMoveForAdaptationRole(const FEvaEnemyAdaptationTuning& Tuning,
    const FVector& PawnLocation, const FVector& TargetLocation)
{
    if (!TargetActor || !GetPawn())
    {
        return false;
    }

    const FVector ToTarget = (TargetLocation - PawnLocation).GetSafeNormal2D();
    if (ToTarget.IsNearlyZero())
    {
        return false;
    }

    switch (Tuning.BehaviorRole)
    {
    case EEvaEnemyBehaviorRole::Flanker:
    {
        if (FMath::FRand() > Tuning.SidestepChance)
        {
            return false;
        }
        const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, ToTarget).GetSafeNormal2D();
        const bool bMoveRight = bPreferRightDetour;
        const FVector Side = bMoveRight ? RightDirection : -RightDirection;
        bPreferRightDetour = !bPreferRightDetour;
        const bool bMoved = MoveToLocationOrDirect(PawnLocation + ToTarget * 220.0f + Side * 560.0f, 80.0f);
        if (bMoved)
        {
            SetCurrentActionIntent(bMoveRight ? TEXT("FLANK RIGHT") : TEXT("FLANK LEFT"));
        }
        return bMoved;
    }
    case EEvaEnemyBehaviorRole::Frontliner:
    {
        const bool bMoved = MoveToActorOrDirect(TargetActor, AttackRange * 0.68f);
        if (bMoved)
        {
            SetCurrentActionIntent(TEXT("HOLD FRONT"));
        }
        return bMoved;
    }
    case EEvaEnemyBehaviorRole::MidRangePressure:
    {
        const float DistanceSq = FVector::DistSquared2D(PawnLocation, TargetLocation);
        if (DistanceSq < FMath::Square(AttackRange * 0.58f))
        {
            const FVector AwayDirection = (PawnLocation - TargetLocation).GetSafeNormal2D();
            const bool bMoved = !AwayDirection.IsNearlyZero() &&
                MoveToLocationOrDirect(PawnLocation + AwayDirection * 300.0f, 90.0f);
            if (bMoved)
            {
                SetCurrentActionIntent(TEXT("KEEP DISTANCE"));
            }
            return bMoved;
        }
        if (DistanceSq > FMath::Square(AttackRange * 1.45f))
        {
            const bool bMoved = MoveToActorOrDirect(TargetActor, AttackRange * 0.82f);
            if (bMoved)
            {
                SetCurrentActionIntent(TEXT("PRESSURE"));
            }
            return bMoved;
        }
        return false;
    }
    case EEvaEnemyBehaviorRole::Searcher:
        if (AActor* HideSpot = FindNearestTaggedActor(TEXT("EvaHideSpot"), TargetLocation))
        {
            const bool bMoved = MoveToLocationOrDirect(HideSpot->GetActorLocation(), 110.0f);
            if (bMoved)
            {
                SetCurrentActionIntent(TEXT("SEARCH LAST SEEN"));
            }
            return bMoved;
        }
        return false;
    case EEvaEnemyBehaviorRole::Ambusher:
        if (AActor* AmbushPoint = FindNearestTaggedActor(TEXT("EvaAmbushPoint"), TargetLocation))
        {
            const bool bMoved = MoveToLocationOrDirect(AmbushPoint->GetActorLocation(), 110.0f);
            if (bMoved)
            {
                SetCurrentActionIntent(TEXT("AMBUSH"));
            }
            return bMoved;
        }
        return false;
    case EEvaEnemyBehaviorRole::CompositeAdaptive:
        if (Tuning.CounteredStyle == EEvaCombatStyle::Ranger || Tuning.CounteredStyle == EEvaCombatStyle::Ghost)
        {
            const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, ToTarget).GetSafeNormal2D();
            const bool bMoveRight = bPreferRightDetour;
            const FVector Side = bMoveRight ? RightDirection : -RightDirection;
            bPreferRightDetour = !bPreferRightDetour;
            const bool bMoved = MoveToLocationOrDirect(PawnLocation + ToTarget * 180.0f + Side * 460.0f, 80.0f);
            if (bMoved)
            {
                const FString Intent = Tuning.CompositeHybridType.IsEmpty() ?
                    FString(bMoveRight ? TEXT("FLANK RIGHT") : TEXT("FLANK LEFT")) : Tuning.CompositeHybridType;
                SetCurrentActionIntent(Intent);
            }
            return bMoved;
        }
        if (Tuning.CounteredStyle == EEvaCombatStyle::Berserker)
        {
            const float DistanceSq = FVector::DistSquared2D(PawnLocation, TargetLocation);
            if (DistanceSq < FMath::Square(AttackRange * 0.90f))
            {
                const FVector AwayDirection = (PawnLocation - TargetLocation).GetSafeNormal2D();
                const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, AwayDirection).GetSafeNormal2D();
                const FVector Side = bPreferRightDetour ? RightDirection : -RightDirection;
                bPreferRightDetour = !bPreferRightDetour;
                const bool bMoved = !AwayDirection.IsNearlyZero() &&
                    MoveToLocationOrDirect(PawnLocation + AwayDirection * 320.0f + Side * 220.0f, 90.0f);
                if (bMoved)
                {
                    const FString Intent = Tuning.CompositeHybridType.IsEmpty() ?
                        FString(TEXT("ANTI-BERSERKER")) : Tuning.CompositeHybridType;
                    SetCurrentActionIntent(Intent);
                }
                return bMoved;
            }
            const bool bMoved = MoveToActorOrDirect(TargetActor, AttackRange * 0.78f);
            if (bMoved)
            {
                const FString Intent = Tuning.CompositeHybridType.IsEmpty() ?
                    FString(TEXT("ANTI-BERSERKER")) : Tuning.CompositeHybridType;
                SetCurrentActionIntent(Intent);
            }
            return bMoved;
        }
        if (Tuning.CounteredStyle == EEvaCombatStyle::Explorer)
        {
            if (AActor* AmbushPoint = FindNearestTaggedActor(TEXT("EvaAmbushPoint"), TargetLocation))
            {
                const bool bMoved = MoveToLocationOrDirect(AmbushPoint->GetActorLocation(), 110.0f);
                if (bMoved)
                {
                    const FString Intent = Tuning.CompositeHybridType.IsEmpty() ?
                        FString(TEXT("ANTI-EXPLORER")) : Tuning.CompositeHybridType;
                    SetCurrentActionIntent(Intent);
                }
                return bMoved;
            }
        }
        return false;
    default:
        return false;
    }
}

bool AEvaZombieAIController::ReissueMoveToTarget(const TCHAR* RepathReason, const bool bAbortCurrentMove)
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !TargetActor || !GetWorld())
    {
        return false;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastMoveRequestTime < 0.35f)
    {
        LogRepathState(RepathReason, EPathFollowingRequestResult::AlreadyAtGoal, LastProgressDistance);
        return false;
    }

    if (bAbortCurrentMove && GetMoveStatus() != EPathFollowingStatus::Idle)
    {
        bInternalRepathAbort = true;
        StopMovement();
        bInternalRepathAbort = false;
    }

    bRecoveringSidestep = false;
    bDirectFallbackActive = false;

    const FVector GoalLocation = TargetActor->GetActorLocation();
    LastMoveRequestGoal = GoalLocation;
    LastRepathTargetLocation = GoalLocation;
    LastMoveRequestTime = Now;

    bIssuingRepathMove = true;
    const EPathFollowingRequestResult::Type MoveResult =
        MoveToActor(TargetActor, AttackRange * 0.75f, true, true, true, nullptr, true);
    bIssuingRepathMove = false;

    if (MoveResult != EPathFollowingRequestResult::Failed)
    {
        ConsecutiveMoveFailures = 0;
        LastProgressSampleLocation = ControlledPawn->GetActorLocation();
        LastProgressSampleTime = Now;
    }
    else
    {
        ++ConsecutiveMoveFailures;
    }

    LogRepathState(RepathReason, MoveResult, LastProgressDistance);
    LogPathDiagnostics(RepathReason ? RepathReason : TEXT("Repath"), GoalLocation, MoveResult);
    return MoveResult != EPathFollowingRequestResult::Failed;
}

bool AEvaZombieAIController::MoveToActorOrDirect(AActor* GoalActor, const float AcceptanceRadius)
{
    if (!bCombatEnabled)
    {
        return false;
    }

    APawn* ControlledPawn = GetPawn();
    if (!GoalActor || !ControlledPawn || !GetWorld())
    {
        return false;
    }

    const FVector GoalLocation = GoalActor->GetActorLocation();
    const float Now = GetWorld()->GetTimeSeconds();
    const UPathFollowingComponent* PathComponent = GetPathFollowingComponent();
    if (!bRecoveringSidestep && GetMoveStatus() == EPathFollowingStatus::Moving &&
        PathComponent && PathComponent->HasValidPath() &&
        PathComponent->GetMoveGoal() == GoalActor &&
        Now - LastMoveRequestTime < 1.25f)
    {
        return true;
    }

    UNavigationPath* DiagnosticPath =
        UNavigationSystemV1::FindPathToActorSynchronously(GetWorld(), ControlledPawn->GetActorLocation(),
            GoalActor, 50.0f, ControlledPawn);
    const bool bHasValidDiagnosticPath = DiagnosticPath && DiagnosticPath->IsValid();
    const int32 PathPointCount = DiagnosticPath ? DiagnosticPath->PathPoints.Num() : 0;

    const EPathFollowingRequestResult::Type MoveResult =
        MoveToActor(GoalActor, AcceptanceRadius, true, true, true, nullptr, true);
    LastMoveRequestGoal = GoalLocation;
    LastMoveRequestTime = Now;

    if (MoveResult != EPathFollowingRequestResult::Failed)
    {
        ConsecutiveMoveFailures = 0;
        bDirectFallbackActive = false;
        LastRepathTargetLocation = GoalLocation;
        if (Now - LastMoveDiagnosticLogTime > 1.25f)
        {
            LastMoveDiagnosticLogTime = Now;
            UE_LOG(LogAdaptiveHorror, Log,
                TEXT("[AI] MoveToActor accepted Controller=%s Pawn=%s GoalActor=%s Acceptance=%.1f PathValid=%s IsPartial=%s PathPoints=%d Result=%s"),
                *GetName(),
                *ControlledPawn->GetName(),
                *GoalActor->GetName(),
                AcceptanceRadius,
                bHasValidDiagnosticPath ? TEXT("true") : TEXT("false"),
                DiagnosticPath && DiagnosticPath->IsPartial() ? TEXT("true") : TEXT("false"),
                PathPointCount,
                EvaMoveRequestResultToString(MoveResult));
            LogPathDiagnostics(TEXT("MoveToActorAccepted"), GoalLocation, MoveResult);
        }
        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[ZombieAttackDiag] Stage=MoveTo Controller=%s Pawn=%s Target=%s Acceptance=%.1f Result=%s Distance=%.1f"),
            *GetName(),
            *ControlledPawn->GetName(),
            *GoalActor->GetName(),
            AcceptanceRadius,
            EvaMoveRequestResultToString(MoveResult),
            FVector::Dist(ControlledPawn->GetActorLocation(), GoalActor->GetActorLocation()));
        return true;
    }

    ++ConsecutiveMoveFailures;
    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[AI] MoveToActor failed Controller=%s Pawn=%s GoalActor=%s Acceptance=%.1f PathValid=%s IsPartial=%s PathPoints=%d Failures=%d"),
        *GetName(),
        *ControlledPawn->GetName(),
        *GoalActor->GetName(),
        AcceptanceRadius,
        bHasValidDiagnosticPath ? TEXT("true") : TEXT("false"),
        DiagnosticPath && DiagnosticPath->IsPartial() ? TEXT("true") : TEXT("false"),
        PathPointCount,
        ConsecutiveMoveFailures);
    LogPathDiagnostics(TEXT("MoveToActorFailed"), GoalLocation, MoveResult);
    return MoveToLocationOrDirect(GoalActor->GetActorLocation(), AcceptanceRadius);
}

bool AEvaZombieAIController::MoveToLocationOrDirect(const FVector& GoalLocation, const float AcceptanceRadius)
{
    if (!bCombatEnabled)
    {
        return false;
    }

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !GetWorld())
    {
        return false;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const UPathFollowingComponent* PathComponent = GetPathFollowingComponent();
    if (!bRecoveringSidestep && GetMoveStatus() == EPathFollowingStatus::Moving &&
        PathComponent && PathComponent->HasValidPath() &&
        FVector::DistSquared2D(LastMoveRequestGoal, GoalLocation) < FMath::Square(90.0f) &&
        Now - LastMoveRequestTime < 0.75f)
    {
        return true;
    }

    LastMoveRequestGoal = GoalLocation;
    LastMoveRequestTime = Now;

    UNavigationPath* DiagnosticPath =
        UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), ControlledPawn->GetActorLocation(),
            GoalLocation, ControlledPawn);
    const bool bHasValidDiagnosticPath = DiagnosticPath && DiagnosticPath->IsValid();
    const int32 PathPointCount = DiagnosticPath ? DiagnosticPath->PathPoints.Num() : 0;

    const EPathFollowingRequestResult::Type MoveResult =
        MoveToLocation(GoalLocation, AcceptanceRadius, true, true, true, true, nullptr, true);
    if (MoveResult != EPathFollowingRequestResult::Failed)
    {
        ConsecutiveMoveFailures = 0;
        bDirectFallbackActive = false;
        if (Now - LastMoveDiagnosticLogTime > 1.25f)
        {
            LastMoveDiagnosticLogTime = Now;
            UE_LOG(LogAdaptiveHorror, Log,
                TEXT("[AI] MoveToLocation accepted Controller=%s Pawn=%s Goal=%s Acceptance=%.1f PathValid=%s IsPartial=%s PathPoints=%d Result=%s"),
                *GetName(),
                *ControlledPawn->GetName(),
                *GoalLocation.ToCompactString(),
                AcceptanceRadius,
                bHasValidDiagnosticPath ? TEXT("true") : TEXT("false"),
                DiagnosticPath && DiagnosticPath->IsPartial() ? TEXT("true") : TEXT("false"),
                PathPointCount,
                EvaMoveRequestResultToString(MoveResult));
            LogPathDiagnostics(TEXT("MoveToLocationAccepted"), GoalLocation, MoveResult);
        }
        return true;
    }

    ++ConsecutiveMoveFailures;
    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[AI] MoveToLocation failed Controller=%s Pawn=%s Goal=%s Acceptance=%.1f PathValid=%s IsPartial=%s PathPoints=%d Failures=%d"),
        *GetName(),
        *ControlledPawn->GetName(),
        *GoalLocation.ToCompactString(),
        AcceptanceRadius,
        bHasValidDiagnosticPath ? TEXT("true") : TEXT("false"),
        DiagnosticPath && DiagnosticPath->IsPartial() ? TEXT("true") : TEXT("false"),
        PathPointCount,
        ConsecutiveMoveFailures);
    LogPathDiagnostics(TEXT("MoveToLocationFailed"), GoalLocation, MoveResult);

    if (bHasValidDiagnosticPath)
    {
        // If navigation can still produce a route, do not override Path Following with direct movement.
        return false;
    }

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

    FString DirectFallbackReason;
    if (!CanUseDirectFallback(MoveDirection, 115.0f, DirectFallbackReason))
    {
        UE_LOG(LogAdaptiveHorror, Warning,
            TEXT("[AIPath] DirectFallback rejected Controller=%s Pawn=%s Goal=%s Reason=%s"),
            *GetName(),
            *ControlledPawn->GetName(),
            *GoalLocation.ToCompactString(),
            *DirectFallbackReason);
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

    FVector SearchOrigin = ControlledPawn->GetActorLocation();
    bool bUsingPartialPathEnd = false;
    const UPathFollowingComponent* PathComponent = GetPathFollowingComponent();
    const FNavPathSharedPtr ActivePath = PathComponent ? PathComponent->GetPath() : nullptr;
    if (ActivePath.IsValid() && ActivePath->IsPartial() && ActivePath->GetPathPoints().Num() > 0)
    {
        SearchOrigin = ActivePath->GetPathPoints().Last().Location;
        bUsingPartialPathEnd = true;
    }

    FVector DirectionToGoal = (GoalLocation - SearchOrigin).GetSafeNormal2D();
    if (DirectionToGoal.IsNearlyZero())
    {
        DirectionToGoal = (GoalLocation - ControlledPawn->GetActorLocation()).GetSafeNormal2D();
    }
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
        const FVector RawCandidateLocation = SearchOrigin + CandidateDirection * 420.0f;
        FVector ProjectedCandidateLocation = FVector::ZeroVector;
        const bool bProjected = ProjectNavigationPoint(RawCandidateLocation, ProjectedCandidateLocation);
        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[AIPath] SidestepCandidate Controller=%s Pawn=%s Goal=%s Raw=%s Projected=%s ProjectedLocation=%s FromPartialPathEnd=%s"),
            *GetName(),
            *ControlledPawn->GetName(),
            *GoalLocation.ToCompactString(),
            *RawCandidateLocation.ToCompactString(),
            bProjected ? TEXT("true") : TEXT("false"),
            *ProjectedCandidateLocation.ToCompactString(),
            bUsingPartialPathEnd ? TEXT("true") : TEXT("false"));

        if (!bProjected)
        {
            continue;
        }

        const EPathFollowingRequestResult::Type MoveResult =
            MoveToLocation(ProjectedCandidateLocation, 70.0f, true, true, true, true, nullptr, true);
        LogPathDiagnostics(TEXT("SidestepMoveToProjectedCandidate"), ProjectedCandidateLocation, MoveResult);
        if (MoveResult != EPathFollowingRequestResult::Failed)
        {
            LastMoveRequestGoal = ProjectedCandidateLocation;
            LastMoveRequestTime = GetWorld()->GetTimeSeconds();
            LastSidestepMoveTime = LastMoveRequestTime;
            bRecoveringSidestep = true;
            bDirectFallbackActive = false;
            ConsecutiveMoveFailures = 0;
            UE_LOG(LogAdaptiveHorror, Log,
                TEXT("[AI] Sidestep detour accepted Controller=%s Pawn=%s Goal=%s ProjectedLocation=%s MoveRequest=%s"),
                *GetName(),
                *ControlledPawn->GetName(),
                *GoalLocation.ToCompactString(),
                *ProjectedCandidateLocation.ToCompactString(),
                EvaMoveRequestResultToString(MoveResult));
            return true;
        }
    }

    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[AIPath] Sidestep detour failed Controller=%s Pawn=%s Goal=%s Note=No projected candidate accepted by MoveToLocation"),
        *GetName(),
        *ControlledPawn->GetName(),
        *GoalLocation.ToCompactString());
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

    FString DirectFallbackReason;
    if (!CanUseDirectFallback(MoveDirection, 115.0f, DirectFallbackReason))
    {
        UE_LOG(LogAdaptiveHorror, Verbose,
            TEXT("[AIPath] DirectFallback movement skipped Controller=%s Pawn=%s Reason=%s"),
            *GetName(),
            *ControlledPawn->GetName(),
            *DirectFallbackReason);
        return false;
    }

    ControlledPawn->AddMovementInput(MoveDirection, 1.0f);
    bDirectFallbackActive = true;
    LastDirectFallbackTime = GetWorld()->GetTimeSeconds();
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

bool AEvaZombieAIController::CanUseDirectFallback(const FVector& DesiredDirection, const float TraceDistance,
    FString& OutReason) const
{
    const APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !GetWorld())
    {
        OutReason = TEXT("NoPawnOrWorld");
        return false;
    }

    const FVector MoveDirection = DesiredDirection.GetSafeNormal2D();
    if (MoveDirection.IsNearlyZero())
    {
        OutReason = TEXT("ZeroDirection");
        return false;
    }

    const FVector TraceStart = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EvaDirectFallbackTrace), false, ControlledPawn);
    if (TargetActor)
    {
        QueryParams.AddIgnoredActor(TargetActor);
    }

    if (TargetActor)
    {
        FHitResult LineOfSightHit;
        const FVector LineOfSightEnd = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
        const bool bLineBlocked = GetWorld()->LineTraceSingleByChannel(LineOfSightHit, TraceStart, LineOfSightEnd,
            ECC_WorldStatic, QueryParams);
        if (bLineBlocked)
        {
            OutReason = FString::Printf(TEXT("LineOfSightBlocked Hit=%s Impact=%s"),
                LineOfSightHit.GetActor() ? *LineOfSightHit.GetActor()->GetName() : TEXT("WorldStatic"),
                *LineOfSightHit.ImpactPoint.ToCompactString());
#if !UE_BUILD_SHIPPING
            DrawDebugLine(GetWorld(), TraceStart, LineOfSightEnd, FColor::Red, false, 0.35f, 0, 2.0f);
#endif
            return false;
        }
    }

    FHitResult ForwardHit;
    const FVector ForwardTraceEnd = TraceStart + MoveDirection * TraceDistance;
    const bool bForwardBlocked = GetWorld()->LineTraceSingleByChannel(ForwardHit, TraceStart, ForwardTraceEnd,
        ECC_WorldStatic, QueryParams);
    if (bForwardBlocked)
    {
        OutReason = FString::Printf(TEXT("ForwardBlocked Hit=%s Impact=%s"),
            ForwardHit.GetActor() ? *ForwardHit.GetActor()->GetName() : TEXT("WorldStatic"),
            *ForwardHit.ImpactPoint.ToCompactString());
#if !UE_BUILD_SHIPPING
        DrawDebugLine(GetWorld(), TraceStart, ForwardTraceEnd, FColor::Red, false, 0.35f, 0, 2.0f);
#endif
        return false;
    }

    OutReason = TEXT("LineOfSightAndForwardClear");
    return true;
}

bool AEvaZombieAIController::ProjectNavigationPoint(const FVector& Point, FVector& OutProjectedLocation) const
{
    OutProjectedLocation = FVector::ZeroVector;
    if (!GetWorld())
    {
        return false;
    }

    UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSystem)
    {
        return false;
    }

    const ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn());
    const FNavAgentProperties* AgentProperties = ControlledCharacter ?
        &ControlledCharacter->GetNavAgentPropertiesRef() :
        nullptr;

    FNavLocation ProjectedNavLocation;
    const bool bProjected = NavSystem->ProjectPointToNavigation(Point, ProjectedNavLocation,
        FVector(260.0f, 260.0f, 420.0f), AgentProperties);
    if (bProjected)
    {
        OutProjectedLocation = ProjectedNavLocation.Location;
    }
    return bProjected;
}

void AEvaZombieAIController::LogPathDiagnostics(const TCHAR* Context, const FVector& GoalLocation,
    const EPathFollowingRequestResult::Type MoveResult) const
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !GetWorld())
    {
        return;
    }

    const UPathFollowingComponent* PathComponent = GetPathFollowingComponent();
    const FNavPathSharedPtr ActivePath = PathComponent ? PathComponent->GetPath() : nullptr;
    const bool bActivePathValid = PathComponent && PathComponent->HasValidPath();
    const bool bActivePathPartial = PathComponent && PathComponent->HasPartialPath();
    const int32 ActivePathPoints = ActivePath.IsValid() ? ActivePath->GetPathPoints().Num() : 0;
    const int32 CurrentPathPointIndex = PathComponent ? PathComponent->GetCurrentPathIndex() : INDEX_NONE;

    UNavigationPath* DiagnosticPath = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),
        ControlledPawn->GetActorLocation(), GoalLocation, ControlledPawn);
    const bool bDiagnosticPathValid = DiagnosticPath && DiagnosticPath->IsValid();
    const bool bDiagnosticPathPartial = DiagnosticPath && DiagnosticPath->IsPartial();
    const int32 DiagnosticPathPoints = DiagnosticPath ? DiagnosticPath->PathPoints.Num() : 0;

    FVector PlayerProjectedLocation = FVector::ZeroVector;
    const bool bPlayerProjected = ProjectNavigationPoint(GoalLocation, PlayerProjectedLocation);
    FVector EnemyProjectedLocation = FVector::ZeroVector;
    const bool bEnemyProjected = ProjectNavigationPoint(ControlledPawn->GetActorLocation(), EnemyProjectedLocation);

    FVector DirectionToGoal = (GoalLocation - ControlledPawn->GetActorLocation()).GetSafeNormal2D();
    if (DirectionToGoal.IsNearlyZero())
    {
        DirectionToGoal = ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
    }
    const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, DirectionToGoal).GetSafeNormal2D();
    const FVector RightCandidate = ControlledPawn->GetActorLocation() + RightDirection * 420.0f;
    const FVector LeftCandidate = ControlledPawn->GetActorLocation() - RightDirection * 420.0f;
    FVector RightProjectedLocation = FVector::ZeroVector;
    FVector LeftProjectedLocation = FVector::ZeroVector;
    const bool bRightProjected = ProjectNavigationPoint(RightCandidate, RightProjectedLocation);
    const bool bLeftProjected = ProjectNavigationPoint(LeftCandidate, LeftProjectedLocation);

    FHitResult DirectTraceHit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EvaEnemyToPlayerTrace), false, ControlledPawn);
    if (TargetActor)
    {
        QueryParams.AddIgnoredActor(TargetActor);
    }
    const FVector TraceStart = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
    const FVector TraceEnd = GoalLocation + FVector(0.0f, 0.0f, 55.0f);
    const bool bDirectTraceBlocked = GetWorld()->LineTraceSingleByChannel(DirectTraceHit, TraceStart, TraceEnd,
        ECC_WorldStatic, QueryParams);

    float CapsuleRadius = -1.0f;
    float NavAgentRadius = -1.0f;
    if (const ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
    {
        CapsuleRadius = ControlledCharacter->GetCapsuleComponent() ?
            ControlledCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() :
            -1.0f;
        NavAgentRadius = ControlledCharacter->GetNavAgentPropertiesRef().AgentRadius;
    }

    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[AIPath] Context=%s Controller=%s Pawn=%s Goal=%s MoveRequest=%s PathFollowingState=%s PathValid=%s IsPartial=%s PathPoints=%d CurrentPathPointIndex=%d DiagnosticPathValid=%s DiagnosticIsPartial=%s DiagnosticPathPoints=%d PlayerNavProjection=%s PlayerProjected=%s EnemyNavProjection=%s EnemyProjected=%s LeftDetourNavProjection=%s LeftProjected=%s RightDetourNavProjection=%s RightProjected=%s EnemyToPlayerTraceBlocked=%s TraceHit=%s CapsuleRadius=%.1f NavAgentRadius=%.1f"),
        Context ? Context : TEXT("None"),
        *GetName(),
        *ControlledPawn->GetName(),
        *GoalLocation.ToCompactString(),
        EvaMoveRequestResultToString(MoveResult),
        PathComponent ? EvaPathStatusToString(PathComponent->GetStatus()) : TEXT("NoPathFollowingComponent"),
        bActivePathValid ? TEXT("true") : TEXT("false"),
        bActivePathPartial ? TEXT("true") : TEXT("false"),
        ActivePathPoints,
        CurrentPathPointIndex,
        bDiagnosticPathValid ? TEXT("true") : TEXT("false"),
        bDiagnosticPathPartial ? TEXT("true") : TEXT("false"),
        DiagnosticPathPoints,
        bPlayerProjected ? TEXT("true") : TEXT("false"),
        *PlayerProjectedLocation.ToCompactString(),
        bEnemyProjected ? TEXT("true") : TEXT("false"),
        *EnemyProjectedLocation.ToCompactString(),
        bLeftProjected ? TEXT("true") : TEXT("false"),
        *LeftProjectedLocation.ToCompactString(),
        bRightProjected ? TEXT("true") : TEXT("false"),
        *RightProjectedLocation.ToCompactString(),
        bDirectTraceBlocked ? TEXT("true") : TEXT("false"),
        DirectTraceHit.GetActor() ? *DirectTraceHit.GetActor()->GetName() : TEXT("None"),
        CapsuleRadius,
        NavAgentRadius);
}

void AEvaZombieAIController::LogRepathState(const TCHAR* RepathReason,
    const EPathFollowingRequestResult::Type MoveResult, const float RecentMoveDistance) const
{
    const APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !GetWorld())
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const UPathFollowingComponent* PathComponent = GetPathFollowingComponent();
    const FAIRequestID CurrentRequestId = PathComponent ? PathComponent->GetCurrentRequestId() : FAIRequestID();
    const bool bPathValid = PathComponent && PathComponent->HasValidPath();
    const bool bPathPartial = PathComponent && PathComponent->HasPartialPath();
    const bool bDirectFallbackRecentlyActive = bDirectFallbackActive && Now - LastDirectFallbackTime <= 0.75f;

    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[AIRepath] Reason=%s Controller=%s Pawn=%s Target=%s SinceMoveTo=%.2f SinceProgress=%.2f RecentMoveDistance=%.1f DirectFallbackActive=%s SidestepActive=%s PathFollowingStatus=%s PathValid=%s IsPartial=%s CurrentMoveRequestID=%u MoveToResult=%s"),
        RepathReason ? RepathReason : TEXT("None"),
        *GetName(),
        *ControlledPawn->GetName(),
        TargetActor ? *TargetActor->GetName() : TEXT("None"),
        Now - LastMoveRequestTime,
        Now - LastMeaningfulProgressTime,
        RecentMoveDistance,
        bDirectFallbackRecentlyActive ? TEXT("true") : TEXT("false"),
        bRecoveringSidestep ? TEXT("true") : TEXT("false"),
        PathComponent ? EvaPathStatusToString(PathComponent->GetStatus()) : TEXT("NoPathFollowingComponent"),
        bPathValid ? TEXT("true") : TEXT("false"),
        bPathPartial ? TEXT("true") : TEXT("false"),
        CurrentRequestId.GetID(),
        EvaMoveRequestResultToString(MoveResult));
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
