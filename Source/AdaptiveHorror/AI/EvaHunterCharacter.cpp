#include "AI/EvaHunterCharacter.h"
#include "AI/EvaHunterAIController.h"
#include "AI/EvaLearningSubsystem.h"
#include "Components/EvaHealthComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Pickups/EvaAnalysisCorePickup.h"

AEvaHunterCharacter::AEvaHunterCharacter()
{
    Tags.AddUnique(TEXT("Hunter"));
    Tags.AddUnique(TEXT("Zombie"));
    BaseMovementSpeed = 420.0f;
    MovementSpeed = BaseMovementSpeed;
    BaseHealth = HunterHP;
    BaseAttackRange = 240.0f;
    BaseAttackDamage = 18.0f;
    AIControllerClass = AEvaHunterAIController::StaticClass();
    AnalysisCorePickupClass = AEvaAnalysisCorePickup::StaticClass();
}

void AEvaHunterCharacter::BeginPlay()
{
    BaseHealth = HunterHP + FMath::Max(0, HunterTier - 1) * 75.0f;
    LearningSpeedMultiplier = 1.0f;
    AnalysisRate = 100.0f;
    Super::BeginPlay();

    if (HealthComponent)
    {
        HealthComponent->SetMaxHealth(BaseHealth, true);
    }
    if (BodyVisual)
    {
        BodyVisual->SetRelativeScale3D(FVector(0.75f, 0.55f, 1.35f));
    }
    if (HeadVisual)
    {
        HeadVisual->SetRelativeScale3D(FVector(0.48f));
    }
    SetPrototypeDebugLabel(FString::Printf(TEXT("HUNTER T%d"), HunterTier), FColor::Red, 54.0f);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->SetHunterState(EEvaHunterState::Deployed, HunterTier);
            Learning->SetLearningSpeedMultiplier(1.0f);
        }
    }
}

void AEvaHunterCharacter::InitializeHunterTier(const int32 NewHunterTier)
{
    HunterTier = FMath::Max(1, NewHunterTier);
    HunterHP = 250.0f + FMath::Max(0, HunterTier - 1) * 75.0f;
    BaseHealth = HunterHP;
    if (HealthComponent)
    {
        HealthComponent->SetMaxHealth(HunterHP, true);
    }
}

void AEvaHunterCharacter::OnHunterDefeated()
{
    if (bHunterDefeated)
    {
        return;
    }

    bHunterDefeated = true;
    AnalysisRate = 0.0f;
    LearningSpeedMultiplier = 0.3f;
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->SetHunterState(EEvaHunterState::DefeatedCooldown, HunterTier);
            Learning->SetLearningSpeedMultiplier(0.3f);
        }
    }

    DropAnalysisCore();
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->NotifyHunterDefeated(this);
    }
#if !UE_BUILD_SHIPPING
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Purple, TEXT("HUNTER defeated: analysis core dropped."));
    }
#endif
}

void AEvaHunterCharacter::OnDefeated()
{
    OnHunterDefeated();
    Super::OnDefeated();
}

void AEvaHunterCharacter::DropAnalysisCore()
{
    if (!GetWorld() || !AnalysisCorePickupClass)
    {
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AEvaAnalysisCorePickup* Core = GetWorld()->SpawnActor<AEvaAnalysisCorePickup>(AnalysisCorePickupClass,
        GetActorLocation() + FVector(0.0f, 0.0f, 60.0f), FRotator::ZeroRotator, SpawnParameters);
    if (Core)
    {
        Core->SourceHunterTier = HunterTier;
    }
}
