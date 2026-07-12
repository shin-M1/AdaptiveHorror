#include "UI/EvaHUD.h"
#include "AI/EvaAdamBossAIController.h"
#include "AI/EvaAdamBossCharacter.h"
#include "AI/EvaLearningSubsystem.h"
#include "AI/EvaTelemetryTypes.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/EvaHealthComponent.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/EvaBossHUDWidget.h"
#include "Weapons/EvaWeaponBase.h"
#include "World/EvaResearchFacilityDirector.h"

void AEvaHUD::BeginPlay()
{
    Super::BeginPlay();
    EnsureBossHUDWidget();
}

void AEvaHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner)
    {
        return;
    }

    UpdateBossHUD();

    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(PlayerOwner->GetPawn());
    if (!Player)
    {
        DrawText(TEXT("Spawning EVA prototype..."), FLinearColor::White, 40.0f, 40.0f);
        return;
    }

    const UEvaHealthComponent* Health = Player->GetHealthComponent();
    const AEvaWeaponBase* Weapon = Player->GetCurrentWeapon();
    const UEvaPlayerTelemetryComponent* Telemetry = Player->GetTelemetryComponent();
    if (!Health || !Telemetry)
    {
        DrawText(TEXT("Initializing EVA player systems..."), FLinearColor::Yellow, 40.0f, 40.0f);
        return;
    }

    const FEvaTelemetrySnapshot Stats = Telemetry->GetTelemetry();

    FString StyleText = TEXT("Unknown");
    switch (Telemetry->ClassifyCombatStyle())
    {
    case EEvaCombatStyle::Berserker: StyleText = TEXT("Berserker"); break;
    case EEvaCombatStyle::Tactician: StyleText = TEXT("Tactician"); break;
    case EEvaCombatStyle::Ranger: StyleText = TEXT("Ranger"); break;
    case EEvaCombatStyle::Ghost: StyleText = TEXT("Ghost"); break;
    case EEvaCombatStyle::Explorer: StyleText = TEXT("Explorer"); break;
    default: break;
    }

    float AnalysisRate = 0.0f;
    float LearningMultiplier = 1.0f;
    EEvaHunterState HunterState = EEvaHunterState::Dormant;
    EEvaAnalysisStage AnalysisStage = EEvaAnalysisStage::Learning;
    EEvaAdaptationDirective AdaptationDirective = EEvaAdaptationDirective::None;
    EEvaEvolutionType NextEvolution = EEvaEvolutionType::None;
    int32 HunterTier = 0;
    UWorld* World = GetWorld();
    if (const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr)
    {
        if (const UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            AnalysisRate = Learning->GetEvaAnalysisRate();
            LearningMultiplier = Learning->GetLearningSpeedMultiplier();
            HunterState = Learning->GetHunterState();
            HunterTier = Learning->GetHunterTier();
            AnalysisStage = Learning->GetAnalysisStage();
            AdaptationDirective = Learning->GetAdaptationDirective();
            NextEvolution = Learning->GetRecommendedEvolutionType();
        }
    }

    auto HunterStateToString = [](const EEvaHunterState State)
    {
        switch (State)
        {
        case EEvaHunterState::Deployed: return FString(TEXT("DEPLOYED"));
        case EEvaHunterState::DefeatedCooldown: return FString(TEXT("DEFEATED / REINSERTING"));
        default: return FString(TEXT("DORMANT"));
        }
    };
    auto StageToString = [](const EEvaAnalysisStage Stage)
    {
        switch (Stage)
        {
        case EEvaAnalysisStage::Adapting: return FString(TEXT("ADAPTING"));
        case EEvaAnalysisStage::Evolving: return FString(TEXT("EVOLVING"));
        default: return FString(TEXT("LEARNING"));
        }
    };
    auto DirectiveToString = [](const EEvaAdaptationDirective Directive)
    {
        switch (Directive)
        {
        case EEvaAdaptationDirective::CounterCloseRange: return FString(TEXT("Keep Distance"));
        case EEvaAdaptationDirective::CounterLongRange: return FString(TEXT("Use Cover"));
        case EEvaAdaptationDirective::CounterStealth: return FString(TEXT("Widen Search"));
        case EEvaAdaptationDirective::CounterExplorer: return FString(TEXT("Ambush Routes"));
        default: return FString(TEXT("None"));
        }
    };
    auto EvolutionToString = [](const EEvaEvolutionType Evolution)
    {
        switch (Evolution)
        {
        case EEvaEvolutionType::Fast: return FString(TEXT("Fast"));
        case EEvaEvolutionType::Armored: return FString(TEXT("Armored"));
        case EEvaEvolutionType::LongArm: return FString(TEXT("Long Arm"));
        case EEvaEvolutionType::Composite: return FString(TEXT("Composite"));
        default: return FString(TEXT("None"));
        }
    };

    float Y = 35.0f;
    const float LineHeight = 24.0f;
    auto DrawStat = [this, &Y, LineHeight](const FString& Text)
    {
        DrawText(Text, FLinearColor(0.8f, 1.0f, 0.85f), 35.0f, Y, nullptr, 1.1f);
        Y += LineHeight;
    };

    DrawStat(FString::Printf(TEXT("HP: %.0f / %.0f"), Health->GetCurrentHealth(), Health->GetMaxHealth()));
    DrawStat(FString::Printf(TEXT("AMMO: %d / %d%s"), Weapon ? Weapon->GetAmmoInMagazine() : 0,
        Weapon ? Weapon->GetReserveAmmo() : 0, Weapon && Weapon->IsReloading() ? TEXT("  RELOADING") : TEXT("")));
    DrawStat(FString::Printf(TEXT("KILLS: %d"), Stats.KillCount));
    DrawStat(FString::Printf(TEXT("ACCURACY: %.1f%%"), Telemetry->GetAccuracy() * 100.0f));
    DrawStat(FString::Printf(TEXT("HEADSHOTS: %.1f%%"), Telemetry->GetHeadshotRate() * 100.0f));
    DrawStat(FString::Printf(TEXT("STYLE: %s"), *StyleText));
    DrawStat(FString::Printf(TEXT("EVA ANALYSIS: %.0f%%  x%.1f"), AnalysisRate, LearningMultiplier));
    DrawStat(FString::Printf(TEXT("EVA STAGE: %s"), *StageToString(AnalysisStage)));
    DrawStat(FString::Printf(TEXT("EVA ADAPT: %s"), *DirectiveToString(AdaptationDirective)));
    DrawStat(FString::Printf(TEXT("NEXT EVOLUTION: %s"), *EvolutionToString(NextEvolution)));
    DrawStat(FString::Printf(TEXT("HUNTER: %s  TIER %d"), *HunterStateToString(HunterState), HunterTier));

    const AEvaPrototypeGameMode* GameMode = World ? World->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr;
    if (GameMode)
    {
        DrawStat(FString::Printf(TEXT("ACTIVE ZOMBIES: %d  HUNTERS: %d  ADAM: %d"),
            GameMode->GetActiveZombieCount(), GameMode->GetActiveHunterCount(), GameMode->GetActiveAdamCount()));
        DrawStat(FString::Printf(TEXT("SPAWN: %s"), *GameMode->GetLastSpawnResult()));
        DrawStat(FString::Printf(TEXT("SPAWN LOC: %s"), *GameMode->GetLastSpawnLocation().ToCompactString()));
        DrawStat(FString::Printf(TEXT("NAVMESH: %s  NAV DEBUG: %s  FALLBACK: %d  STUCK: %d"),
            GameMode->IsNavMeshAvailable() ? TEXT("YES") : TEXT("NO"),
            GameMode->IsNavigationDebugVisible() ? TEXT("ON") : TEXT("OFF"),
            GameMode->GetFallbackMovementCount(),
            GameMode->GetStuckEnemyCount()));
    }
    const AEvaResearchFacilityDirector* Director = GameMode ? GameMode->GetResearchDirector() : nullptr;
    if (Director)
    {
        DrawStat(FString::Printf(TEXT("ZONE: %s"), *Director->GetCurrentZoneName()));
        DrawStat(FString::Printf(TEXT("OBJECTIVE: %s"), *Director->GetObjectiveText()));
        DrawStat(FString::Printf(TEXT("EVA LOGS: %d / 5"), Director->GetCollectedStoryLogCount()));
    }

    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    DrawLine(CenterX - 8.0f, CenterY, CenterX + 8.0f, CenterY, FLinearColor::White, 1.5f);
    DrawLine(CenterX, CenterY - 8.0f, CenterX, CenterY + 8.0f, FLinearColor::White, 1.5f);

    if (GameMode && GameMode->IsGameOver())
    {
        DrawText(TEXT("GAME OVER"), FLinearColor::Red, CenterX - 120.0f, CenterY - 40.0f, nullptr, 2.0f);
        DrawText(TEXT("Restoring last checkpoint..."), FLinearColor::White, CenterX - 150.0f, CenterY + 15.0f);
    }

    if (GameMode && GameMode->ShouldDisplayDebugStatusMessage())
    {
        DrawText(GameMode->GetDebugStatusMessage(), FLinearColor(0.3f, 0.95f, 1.0f),
            CenterX - 320.0f, 85.0f, nullptr, 1.15f);
    }

    if (Director && Director->ShouldDisplayStoryLog())
    {
        const float LogX = CenterX - 360.0f;
        const float LogY = Canvas->ClipY - 150.0f;
        DrawText(FString::Printf(TEXT("EVA LOG: %s"), *Director->GetLastStoryLogTitle()),
            FLinearColor(0.4f, 0.9f, 1.0f), LogX, LogY, nullptr, 1.2f);
        DrawText(Director->GetLastStoryLogBody(), FLinearColor::White, LogX, LogY + 30.0f, nullptr, 0.95f);
    }

    if (GameMode && GameMode->IsStageClear())
    {
        const float ResultX = CenterX - 260.0f;
        const float ResultY = CenterY - 150.0f;
        DrawText(TEXT("STAGE CLEAR"), FLinearColor(0.3f, 1.0f, 0.45f), ResultX, ResultY, nullptr, 2.0f);
        DrawText(FString::Printf(TEXT("Kills: %d"), Stats.KillCount), FLinearColor::White, ResultX, ResultY + 55.0f);
        DrawText(FString::Printf(TEXT("Accuracy: %.1f%%"), Telemetry->GetAccuracy() * 100.0f),
            FLinearColor::White, ResultX, ResultY + 80.0f);
        DrawText(FString::Printf(TEXT("Headshot Rate: %.1f%%"), Telemetry->GetHeadshotRate() * 100.0f),
            FLinearColor::White, ResultX, ResultY + 105.0f);
        DrawText(FString::Printf(TEXT("EVA Analysis: %.0f%%"), AnalysisRate), FLinearColor::White, ResultX, ResultY + 130.0f);
        DrawText(FString::Printf(TEXT("Hunter Defeats: %d"), GameMode->GetHunterDefeatCount()),
            FLinearColor::White, ResultX, ResultY + 155.0f);
        DrawText(FString::Printf(TEXT("Combat Style: %s"), *StyleText), FLinearColor::White, ResultX, ResultY + 180.0f);
        DrawText(TEXT("TODO: Return to title / demo menu."), FLinearColor::Yellow, ResultX, ResultY + 215.0f);
    }
}

void AEvaHUD::EnsureBossHUDWidget()
{
    if (BossHUDWidget || !PlayerOwner)
    {
        return;
    }

    TSubclassOf<UEvaBossHUDWidget> WidgetClass = BossHUDWidgetClass;
    if (!WidgetClass)
    {
        WidgetClass = UEvaBossHUDWidget::StaticClass();
    }

    BossHUDWidget = CreateWidget<UEvaBossHUDWidget>(PlayerOwner, WidgetClass);
    if (BossHUDWidget)
    {
        BossHUDWidget->AddToViewport(30);
        BossHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void AEvaHUD::UpdateBossHUD()
{
    UWorld* World = GetWorld();
    const AEvaPrototypeGameMode* GameMode = World ? World->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr;
    const AEvaResearchFacilityDirector* Director = GameMode ? GameMode->GetResearchDirector() : nullptr;
    AEvaAdamBossCharacter* Adam = Director && Director->IsAdamEncounterActive() ? Director->GetActiveAdam() : nullptr;
    const UEvaHealthComponent* AdamHealth = Adam ? Adam->GetHealthComponent() : nullptr;
    const bool bShowBossHUD = Director && Director->IsAdamEncounterActive() && Adam && AdamHealth &&
        !AdamHealth->IsDead() && GameMode && !GameMode->IsStageClear();

    if (!bShowBossHUD)
    {
        if (BossHUDWidget)
        {
            BossHUDWidget->ClearBossSnapshot();
            BossHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }

    EnsureBossHUDWidget();
    if (!BossHUDWidget)
    {
        return;
    }

    if (Canvas)
    {
        BossHUDWidget->SetDesiredSizeInViewport(FVector2D(Canvas->ClipX, Canvas->ClipY));
        BossHUDWidget->SetPositionInViewport(FVector2D::ZeroVector, false);
    }

    const AEvaPlayerCharacter* Player = PlayerOwner ? Cast<AEvaPlayerCharacter>(PlayerOwner->GetPawn()) : nullptr;
    const AEvaAdamBossAIController* AdamController = Cast<AEvaAdamBossAIController>(Adam->GetController());
    const float TargetDistance = AdamController ? AdamController->GetCurrentTargetDistance() :
        (Player ? FVector::Distance(Adam->GetActorLocation(), Player->GetActorLocation()) : -1.0f);
    const int32 SummonCount = FMath::Max(Adam->GetLastSummonCount(),
        AdamController ? AdamController->GetLastSummonCount() : 0);

    BossHUDWidget->SetBossSnapshot(true, TEXT("ADAM"), AdamHealth->GetHealthPercent(),
        AdamHealth->GetCurrentHealth(), AdamHealth->GetMaxHealth(),
        Adam->IsPhaseTwo() ? TEXT("Phase 2") : TEXT("Phase 1"),
        AdamController ? AdamController->GetCurrentAdamState() : TEXT("No Controller"),
        TargetDistance,
        SummonCount,
        AdamController ? AdamController->GetLastAdamEvent() : TEXT("None"),
        Adam->IsDebugBossEnabled());
    BossHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}
