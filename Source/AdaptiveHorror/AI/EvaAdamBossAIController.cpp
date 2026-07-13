#include "AI/EvaAdamBossAIController.h"
#include "AI/EvaAdamBossCharacter.h"
#include "AI/EvaLearningSubsystem.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

AEvaAdamBossAIController::AEvaAdamBossAIController()
{
    PrimaryActorTick.TickInterval = 0.12f;
    ConfigureCombat(260.0f, 25.0f, 2.0f);
    ConfigurePerception(2800.0f, 2200.0f);
}

void AEvaAdamBossAIController::Tick(const float DeltaSeconds)
{
    AAIController::Tick(DeltaSeconds);

    if (!GetPawn() || !GetWorld())
    {
        SetAdamDebugState(TEXT("No Pawn"));
        return;
    }

    if (!TargetActor)
    {
        SetAdamDebugState(TEXT("Acquiring Target"));
        if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
        {
            SetPlayerTarget(PlayerController->GetPawn());
        }
    }

    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(TargetActor);
    if (!Player || Player->IsDead())
    {
        TargetActor = nullptr;
        CurrentTargetDistance = -1.0f;
        SetAdamDebugState(TEXT("No Valid Target"));
        ClearFocus(EAIFocusPriority::Gameplay);
        StopMovement();
        return;
    }

    CurrentTargetDistance = FVector::Distance(GetPawn()->GetActorLocation(), TargetActor->GetActorLocation());
    SetFocus(TargetActor);

    if (CanAttackTarget())
    {
        StopMovement();
        TryAttackTarget();
        TryRoarSummon();
        return;
    }

    if (TryChargeAttack())
    {
        return;
    }

    const bool bRoared = TryRoarSummon();
    if (!bRoared)
    {
        SetAdamDebugState(TEXT("Chasing"));
    }
    MoveToActorOrDirect(TargetActor, AttackRange * 0.75f);
}

void AEvaAdamBossAIController::TryAttackTarget()
{
    SetAdamDebugState(TEXT("Attack"), TEXT("Attack started"));
    Super::TryAttackTarget();
}

bool AEvaAdamBossAIController::TryChargeAttack()
{
    AEvaAdamBossCharacter* Adam = Cast<AEvaAdamBossCharacter>(GetPawn());
    if (!Adam || !TargetActor || !GetWorld())
    {
        return false;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastChargeTime < Adam->GetChargeCooldown())
    {
        return false;
    }

    const float Distance = FVector::Distance(Adam->GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance < 520.0f || Distance > 1600.0f)
    {
        return false;
    }

    LastChargeTime = Now;
    SetAdamDebugState(TEXT("Charge"), TEXT("Charge started"));
    Adam->PlayAdamChargeFeedback();
    MoveToLocationOrDirect(TargetActor->GetActorLocation(), 60.0f);
    UGameplayStatics::ApplyDamage(TargetActor, Adam->GetChargeDamage(), this, Adam, UDamageType::StaticClass());
    return true;
}

bool AEvaAdamBossAIController::TryRoarSummon()
{
    AEvaAdamBossCharacter* Adam = Cast<AEvaAdamBossCharacter>(GetPawn());
    if (!Adam || !GetWorld())
    {
        return false;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastRoarTime < RoarCooldown)
    {
        return false;
    }

    bool bSummonEvolved = Adam->IsPhaseTwo();
    if (const UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (const UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            bSummonEvolved = bSummonEvolved || Learning->GetEvaAnalysisRate() >= 60.0f;
        }
    }

    LastRoarTime = Now;
    const int32 SummonCount = Adam->IsPhaseTwo() ? 3 : 2;
    SetAdamDebugState(TEXT("Roar / Summon"), TEXT("Roar started / Summon started"), SummonCount);
    Adam->PlayAdamRoarFeedback();
    Adam->SpawnRoarMinions(SummonCount, bSummonEvolved);
    return true;
}

void AEvaAdamBossAIController::SetAdamDebugState(const FString& NewState, const FString& NewEvent,
    const int32 NewSummonCount)
{
    if (!NewState.IsEmpty())
    {
        CurrentAdamState = NewState;
    }
    if (!NewEvent.IsEmpty())
    {
        LastAdamEvent = NewEvent;
    }
    if (NewSummonCount != INDEX_NONE)
    {
        LastSummonCount = FMath::Max(0, NewSummonCount);
    }
}
