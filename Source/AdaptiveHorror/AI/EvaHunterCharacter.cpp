#include "AI/EvaHunterCharacter.h"
#include "AdaptiveHorror.h"
#include "AI/EvaHunterAIController.h"
#include "AI/EvaLearningSubsystem.h"
#include "Audio/EvaAudioFunctionLibrary.h"
#include "Components/EvaHealthComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Pickups/EvaAnalysisCorePickup.h"

namespace
{
FString EvaHunterCounterToShortLabel(const EEvaHunterCounterType CounterType)
{
    switch (CounterType)
    {
    case EEvaHunterCounterType::AntiBerserker:
        return TEXT("ANTI-BERSERKER");
    case EEvaHunterCounterType::AntiRanger:
        return TEXT("ANTI-RANGER");
    case EEvaHunterCounterType::AntiGhost:
        return TEXT("ANTI-GHOST");
    case EEvaHunterCounterType::AntiExplorer:
        return TEXT("ANTI-EXPLORER");
    default:
        return TEXT("OBSERVE");
    }
}
}

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
    if (LeftArmVisual)
    {
        LeftArmVisual->SetRelativeLocation(FVector(0.0f, -58.0f, 44.0f));
        LeftArmVisual->SetRelativeScale3D(FVector(0.18f, 0.18f, 1.05f));
    }
    if (RightArmVisual)
    {
        RightArmVisual->SetRelativeLocation(FVector(0.0f, 58.0f, 44.0f));
        RightArmVisual->SetRelativeScale3D(FVector(0.18f, 0.18f, 1.05f));
    }
    if (LeftLegVisual)
    {
        LeftLegVisual->SetRelativeLocation(FVector(0.0f, -22.0f, -58.0f));
        LeftLegVisual->SetRelativeScale3D(FVector(0.16f, 0.14f, 0.86f));
    }
    if (RightLegVisual)
    {
        RightLegVisual->SetRelativeLocation(FVector(0.0f, 22.0f, -58.0f));
        RightLegVisual->SetRelativeScale3D(FVector(0.16f, 0.14f, 0.86f));
    }
    if (LeftShoulderVisual)
    {
        LeftShoulderVisual->SetRelativeLocation(FVector(0.0f, -62.0f, 78.0f));
        LeftShoulderVisual->SetRelativeScale3D(FVector(0.34f, 0.18f, 0.26f));
    }
    if (RightShoulderVisual)
    {
        RightShoulderVisual->SetRelativeLocation(FVector(0.0f, 62.0f, 78.0f));
        RightShoulderVisual->SetRelativeScale3D(FVector(0.34f, 0.18f, 0.26f));
    }
    ApplyPrototypeVisualColor(FLinearColor(0.02f, 0.02f, 0.025f, 1.0f));
    SetPrototypeDebugLabel(FString::Printf(TEXT("HUNTER T%d"), HunterTier), FColor::Red, 54.0f);
    LogPrototypeDebugLabelState(TEXT("HunterBeginPlayFinal"));
    UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, GetActorLocation(), 55.0f, 0.55f, 0.58f);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->UpdateAdaptationProfile(false);
            Learning->SetHunterState(EEvaHunterState::Deployed, HunterTier);
            Learning->SetLearningSpeedMultiplier(1.0f);
            SetHunterCounterType(Learning->GetHunterCounterTypeForTier(HunterTier));
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

void AEvaHunterCharacter::SetHunterCounterType(const EEvaHunterCounterType NewCounterType)
{
    HunterCounterType = NewCounterType;
    SetPrototypeDebugLabel(FString::Printf(TEXT("HUNTER T%d [%s]"),
        HunterTier,
        *EvaHunterCounterToShortLabel(HunterCounterType)), FColor::Red, 46.0f);
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[HunterAdapt] Label Actor=%s Tier=%d Counter=%s"),
        *GetName(),
        HunterTier,
        *UEnum::GetValueAsString(HunterCounterType));
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
            Learning->RecordHunterDefeatedProfile();
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
