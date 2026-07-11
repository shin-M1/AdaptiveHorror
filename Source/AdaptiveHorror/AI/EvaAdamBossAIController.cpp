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

    TryRoarSummon();
    MoveToActorOrDirect(TargetActor, AttackRange * 0.75f);
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
    Adam->SpawnRoarMinions(Adam->IsPhaseTwo() ? 3 : 2, bSummonEvolved);
    return true;
}
